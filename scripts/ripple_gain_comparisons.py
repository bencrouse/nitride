#!/usr/bin/env python3
"""Audit Ripple makeup without confusing loudness with color or hiding peak loss."""
from functools import lru_cache
import argparse
import json
import math
from pathlib import Path
import shutil

import evaluation as ev
import factory_filter_comparisons as audio
from ripple_audition_comparisons import magnitude_profile


@lru_cache(maxsize=65)
def shaped_ratio(index):
    drive = index/64; gain = 1+15*drive*drive; energy = 0.
    for n in range(8192):
        x = math.sin(2*math.pi*(n+.5)/8192)
        y = (1-drive)*x+drive*math.tanh(gain*x)/math.sqrt(gain)
        energy += y*y
    return math.sqrt(energy/4096)


def expected_gain(values):
    dry, drive, feedback = (values['ripple_'+key] for key in ('dry', 'drive', 'feedback'))
    if values['ripple_mode'] == 1:
        g = .94*abs(feedback); power = (1-g)/(1+g)
    else:
        position = drive*64; index = min(int(position), 63)
        ratio = shaped_ratio(index)+(position-index)*(shaped_ratio(index+1)-shaped_ratio(index))
        power = ratio*ratio
    return 1/math.sqrt(max(.0625, power+dry*dry))


def stage_reference(batch, output):
    batch, output = Path(batch), Path(output); output.mkdir(parents=True, exist_ok=False)
    metadata = json.loads((batch/'batch.json').read_text())
    for entry in metadata['sketches']:
        stem = entry['file']; preset = json.loads((batch/(stem+'.nitridepreset')).read_text())
        preset['values'].update(cutoff=18000., resonance=0., filter_amount=0., filter_key_tracking=0., filter_velocity=0., filter_fm_depth=0.,
                                lfo_cutoff_enabled=0., ripple_type=0., ripple_mode=0., ripple_dry=0., ripple_drive=0., ripple_feedback=0.)
        (output/(stem+'.nitridepreset')).write_text(json.dumps(preset, indent=2)+'\n')
        shutil.copy2(batch/(stem+'.mid'), output/(stem+'.mid'))
    (output/'batch.json').write_text(json.dumps(metadata, indent=2)+'\n')


def undo_existing_ceiling(bank, output_db):
    gain = 10**(output_db/20); result = []
    # The frozen auditions have Space/tremolo off. Omit initial family fade, then
    # invert the EXISTING .8*tanh bus ceiling to inspect the signal before it.
    # JUCE's pinned Reverb multiplies dryLevel=1 by TWO even with Space mix=0.
    # That legacy gain is deliberately untouched by the Ripple-only correction.
    for n in range(4800, len(bank)//2):
        sample = bank[2*n]/8388608
        if abs(sample) >= .9499: raise ValueError('Final renderer hard-ceiling contact invalidates the gain audit')
        x = sample/gain/2/.8
        if abs(x) >= .9999: raise ValueError('Insufficient existing soft-ceiling headroom for an honest gain audit')
        result.append(math.atanh(x))
    if not result: raise ValueError('Insufficient audio after the initial family fade')
    return result


def require_levels(rows, reference_gate=False):
    if len(rows) != 6: raise ValueError('Six complete preset audits required')
    for row in rows:
        if not math.isfinite(row['peak_headroom_dbfs']) or row['peak_headroom_dbfs'] < 3:
            raise ValueError('Less than 3 dB measured peak headroom: '+row['preset'])
        if reference_gate and not -6 <= row['corrected_vs_unity_reference_db'] <= 6:
            raise ValueError('Corrected r2 remains outside the +/-6 dB reference-level window: '+row['preset'])


def compare(batch, baseline, corrected, reference, output):
    batch, baseline, corrected, reference, output = map(Path, (batch, baseline, corrected, reference, output))
    output.mkdir(parents=True, exist_ok=False); rows = []
    reports = {key: {m['name']: m for m in json.loads((folder/'report.json').read_text())}
               for key, folder in (('baseline', baseline), ('corrected', corrected), ('reference', reference))}
    metadata = json.loads((batch/'batch.json').read_text())
    for entry in metadata['sketches']:
        stem = entry['file']; source = json.loads((batch/(stem+'.nitridepreset')).read_text()); values = source['values']
        if values['space_mix'] or values['lfo_amplitude_enabled']: raise ValueError('Ceiling audit requires dry, unmodulated master gain')
        for folder in (baseline, corrected):
            if not ev.same_serialized_preset(json.loads((folder/(stem+'.nitridepreset')).read_text()), source): raise ValueError('Preset conditions changed')
            if (folder/(stem+'.mid')).read_bytes() != (batch/(stem+'.mid')).read_bytes(): raise ValueError('Frozen MIDI changed')
        metrics = {key: data[source['name']] for key, data in reports.items()}
        for metric in metrics.values():
            if metric['faults'] or not metric['deterministic'] or metric['otherBlockMaximumError'] >= 1e-5: raise ValueError('Render guard failed')
        old, new, flat = (audio.pcm(folder/(stem+'-raw.wav')) for folder in (baseline, corrected, reference))
        before, after = (undo_existing_ceiling(bank, values['output']) for bank in (old, new))
        expected = 2*expected_gain(values)  # eval-048 had a fixed .5 output factor.
        measured = sum(a*b for a, b in zip(before, after))/sum(a*a for a in before)
        error = sum((b-expected*a)**2 for a, b in zip(before, after))/max(sum(b*b for b in after), 1e-30)
        residual = 10*math.log10(max(error, 1e-30))
        if abs(20*math.log10(measured/expected)) > .005 or residual > -65: raise ValueError('Makeup altered the pre-ceiling color/trajectory or follows the audio')
        target = min(-24., *(m['rawActiveRmsDbfs']-m['rawPeakDbfs']-3 for m in metrics.values()))
        # Flat -> old -> corrected; matching levels is only for the listening copy.
        audio.sequence(output/(stem+' - reference then old then corrected.wav'), [flat, old, new],
                       [10**((target-metrics[key]['rawActiveRmsDbfs'])/20) for key in ('reference', 'baseline', 'corrected')])
        rows.append({'preset': source['name'], 'predicted_makeup_vs_eval048_db': 20*math.log10(expected),
            'measured_pre_ceiling_gain_db': 20*math.log10(measured), 'pre_ceiling_fixed_gain_residual_db': residual,
            'post_ceiling_gain_aligned_spectral_residual_db': audio.residual(magnitude_profile(old), magnitude_profile(new)),
            'raw_active_rms_gain_db': metrics['corrected']['rawActiveRmsDbfs']-metrics['baseline']['rawActiveRmsDbfs'],
            'corrected_vs_unity_reference_db': metrics['corrected']['rawActiveRmsDbfs']-metrics['reference']['rawActiveRmsDbfs'],
            'peak_headroom_dbfs': -metrics['corrected']['rawPeakDbfs'], 'target_active_rms_dbfs': target, 'metrics': metrics, 'listening_approved': False})
    require_levels(rows, reference_gate=metadata['revision']=='r2')
    result = {'format': 'nitride-ripple-gain-audit', 'version': 1, 'presets': rows,
        'note': 'Same frozen MIDI/conditions/output settings. Pure control-only makeup proven before the existing unchanged tanh bus ceiling; its downstream color/crest effects are reported separately. Fixed gain is not AGC or loudness locking. No new limiter, preset rewrite or notch fill. Matched active-RMS listening copies are not LUFS/approval; raw levels and peak headroom remain visible.'}
    (output/'report.json').write_text(json.dumps(result, indent=2)+'\n')
    (output/'LISTEN.txt').write_text(result['note']+'\nStart quietly: native output is deliberately louder.\n')
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__); sub = parser.add_subparsers(dest='action', required=True)
    p = sub.add_parser('stage'); p.add_argument('batch'); p.add_argument('output')
    p = sub.add_parser('compare')
    for field in ('batch', 'baseline', 'corrected', 'reference', 'output'): p.add_argument(field)
    args = parser.parse_args()
    if args.action == 'stage': stage_reference(args.batch, args.output)
    else: print(json.dumps(compare(args.batch, args.baseline, args.corrected, args.reference, args.output), indent=2))


if __name__ == '__main__': main()
