SHELL := /bin/zsh
CMAKE ?= $(shell command -v cmake)
BUILD_DIR ?= build
CONFIG ?= Debug
JUCE_DIR ?=
JOBS ?= 6
AU_TYPE ?= aumu
AU_SUBTYPE ?= NT01
AU_MANUFACTURER ?= NTRD
AU_COMPONENT ?= nitride.component
AU_INSTALL_DIR ?= $(HOME)/Library/Audio/Plug-Ins/Components
AU_BUILD_PATH := $(BUILD_DIR)/nitride_artefacts/$(CONFIG)/AU/$(AU_COMPONENT)
AU_PRESET_TOOL ?= $(BUILD_DIR)/nitride_au_presets
AU_PRESET_INSTALL_DIR ?= $(HOME)/Library/Audio/Presets/nitride/nitride
PERF_BUILD_DIR ?= build/performance
PERF_ARGS ?= --output "$(PERF_BUILD_DIR)/latest.json"
REVIEW_BATCH ?= presets/review/batch-01/r1
REVIEW_OUTPUT ?= $(abspath $(BUILD_DIR))/preset-review/batch-01/r1

.PHONY: help configure build test check check-full benchmark render-pitch preset-review studies gestures deformation review render-studies render-studies-02 render-studies-03 render-studies-04 render-studies-05 install-au validate-au reload-au standalone clean
.PHONY: evaluation-status evaluation-verify evaluation-backup
.PHONY: factory-selection factory-pilot factory-install install-au-presets
help:
	@echo "make build        Build AU + Standalone"
	@echo "make test         Check native UI, automation/state and DSP"
	@echo "make check        Run the reproducible local regression workflow"
	@echo "make check-full   Run clean builds, diagnostics and installed AU checks locally"
	@echo "make benchmark    Measure Release processor rendering; override PERF_ARGS"
	@echo "make render-pitch Render musical Glide/Autobend comparisons under build/pitch-motion"
	@echo "make preset-review Export pad/bass r1 sketches, MIDI and production-rendered WAVs"
	@echo "make evaluation-status Resume evaluation from its persistent journal"
	@echo "make evaluation-verify Check journal/evidence integrity"
	@echo "make factory-selection Check the proposed, separate release selection"
	@echo "make factory-pilot Export and verify a NEW native-AU preset checkpoint (set FACTORY_OUTPUT)"
	@echo "make factory-install Install verified categories at the preset root (set FACTORY_PACKAGE; requires host confirmation)"
	@echo "make standalone   Build and open the native app"
	@echo "make studies      Build and open the sound comparison app"
	@echo "make gestures     Build and open the playable control-surface study"
	@echo "make deformation  Open the separate visible-object interaction prototype"
	@echo "make review       Open the full dark/orange instrument mockup"
	@echo "make render-studies  Write RMS-matched WAV comparisons under build/sound-studies"
	@echo "make render-studies-02  Write the second comparison round under build/sound-studies-02"
	@echo "make render-studies-03  Compare original and note-coupled phase/delay at 30/70/100%"
	@echo "make render-studies-04  Compare adaptive oscillator coupling against F at 50%"
	@echo "make render-studies-05  Compare G with the extended network stress range"
	@echo "make reload-au    Build, install, and validate AU + missing factory presets"
	@echo "make install-au-presets Build/install AU and add missing native factory presets; existing saves preserved"
	@echo "Overrides: JUCE_DIR, BUILD_DIR, CONFIG, JOBS, AU_INSTALL_DIR, AU_PRESET_INSTALL_DIR"

configure:
	$(CMAKE) -S . -B "$(BUILD_DIR)" -DCMAKE_BUILD_TYPE=$(CONFIG) -DJUCE_DIR="$(JUCE_DIR)"

build: configure
	$(CMAKE) --build "$(BUILD_DIR)" --config $(CONFIG) --parallel $(JOBS)

test: build
	ctest --test-dir "$(BUILD_DIR)" -C $(CONFIG) --output-on-failure

check:
	python3 scripts/check.py

check-full:
	python3 scripts/check.py --full

benchmark:
	$(CMAKE) -S . -B "$(PERF_BUILD_DIR)" -DCMAKE_BUILD_TYPE=Release -DJUCE_DIR="$(JUCE_DIR)"
	$(CMAKE) --build "$(PERF_BUILD_DIR)" --target nitride_perf --config Release --parallel $(JOBS)
	"$(PERF_BUILD_DIR)/nitride_perf_artefacts/Release/nitride_perf" $(PERF_ARGS)

render-pitch: configure
	$(CMAKE) --build "$(BUILD_DIR)" --target nitride_pitch_tests --config $(CONFIG) --parallel $(JOBS)
	"$(BUILD_DIR)/nitride_pitch_tests_artefacts/$(CONFIG)/nitride_pitch_tests" --render-demo "$(abspath $(BUILD_DIR))/pitch-motion"

preset-review:
	$(CMAKE) --preset local-release
	$(CMAKE) --build --preset local-release --target nitride_preset_review
	"build/local-release/nitride_preset_review_artefacts/Release/nitride_preset_review" "$(REVIEW_BATCH)" "$(REVIEW_OUTPUT)"

evaluation-status:
	python3 scripts/evaluation.py status

evaluation-verify:
	python3 scripts/evaluation.py verify

evaluation-backup:
	@test -n "$(BACKUP)" || (echo 'Specify BACKUP=/path/outside/evaluation/snapshot.zip' && exit 1)
	python3 scripts/evaluation.py backup "$(BACKUP)"

factory-selection:
	python3 scripts/factory_presets.py selection

factory-pilot:
	@test -n "$(FACTORY_OUTPUT)" || (echo 'Specify a NEW FACTORY_OUTPUT=/path/to/pilot' && exit 1)
	$(CMAKE) --preset local-release
	$(CMAKE) --build --preset local-release --target nitride_au_presets
	python3 scripts/factory_presets.py export "$(FACTORY_OUTPUT)"
	python3 scripts/factory_presets.py verify "$(FACTORY_OUTPUT)"

factory-install:
	@test -n "$(FACTORY_PACKAGE)" || (echo 'Specify FACTORY_PACKAGE=/path/to/verified/candidate' && exit 1)
	python3 scripts/factory_presets.py install-factory "$(FACTORY_PACKAGE)"

install-au: build
	mkdir -p "$(AU_INSTALL_DIR)"
	cp -R "$(AU_BUILD_PATH)" "$(AU_INSTALL_DIR)/"
	codesign --force --sign - --timestamp=none "$(AU_INSTALL_DIR)/$(AU_COMPONENT)"
	touch "$(AU_INSTALL_DIR)/$(AU_COMPONENT)"

validate-au:
	launchctl asuser $(shell id -u) /usr/bin/auval -v $(AU_TYPE) $(AU_SUBTYPE) $(AU_MANUFACTURER)

install-au-presets: install-au
	$(CMAKE) --build "$(BUILD_DIR)" --config $(CONFIG) --parallel $(JOBS) --target nitride_au_presets
	python3 scripts/factory_presets.py deploy --component "$(AU_INSTALL_DIR)/$(AU_COMPONENT)" --tool "$(AU_PRESET_TOOL)" --destination "$(AU_PRESET_INSTALL_DIR)"

reload-au: install-au-presets
	$(MAKE) validate-au

standalone: build
	codesign --force --sign - --timestamp=none "$(BUILD_DIR)/nitride_artefacts/$(CONFIG)/Standalone/nitride.app"
	open "$(BUILD_DIR)/nitride_artefacts/$(CONFIG)/Standalone/nitride.app"

studies: configure
	$(CMAKE) --build "$(BUILD_DIR)" --target nitride_studies --config $(CONFIG) --parallel $(JOBS)
	open "$(BUILD_DIR)/nitride_studies_artefacts/$(CONFIG)/Nitride Sound Studies.app"

gestures: configure
	$(CMAKE) --build "$(BUILD_DIR)" --target nitride_gestures --config $(CONFIG) --parallel $(JOBS)
	open "$(BUILD_DIR)/nitride_gestures_artefacts/$(CONFIG)/Nitride Gesture Study.app"

deformation: configure
	$(CMAKE) --build "$(BUILD_DIR)" --target nitride_deformation --config $(CONFIG) --parallel $(JOBS)
	open "$(BUILD_DIR)/nitride_deformation_artefacts/$(CONFIG)/Nitride Deformation Study.app"

review: configure
	$(CMAKE) --build "$(BUILD_DIR)" --target nitride_review --config $(CONFIG) --parallel $(JOBS)
	open "$(BUILD_DIR)/nitride_review_artefacts/$(CONFIG)/Nitride Instrument Review.app"

render-studies: configure
	$(CMAKE) --build "$(BUILD_DIR)" --target nitride_study_render --config $(CONFIG) --parallel $(JOBS)
	"$(BUILD_DIR)/nitride_study_render" "$(BUILD_DIR)/sound-studies"

render-studies-02: configure
	$(CMAKE) --build "$(BUILD_DIR)" --target nitride_study_render --config $(CONFIG) --parallel $(JOBS)
	"$(BUILD_DIR)/nitride_study_render" "$(BUILD_DIR)/sound-studies-02" --study-02

render-studies-03: configure
	$(CMAKE) --build "$(BUILD_DIR)" --target nitride_study_render --config $(CONFIG) --parallel $(JOBS)
	"$(BUILD_DIR)/nitride_study_render" "$(BUILD_DIR)/sound-studies-03" --study-03

render-studies-04: configure
	$(CMAKE) --build "$(BUILD_DIR)" --target nitride_study_render --config $(CONFIG) --parallel $(JOBS)
	"$(BUILD_DIR)/nitride_study_render" "$(BUILD_DIR)/sound-studies-04" --study-04

render-studies-05: configure
	$(CMAKE) --build "$(BUILD_DIR)" --target nitride_study_render --config $(CONFIG) --parallel $(JOBS)
	"$(BUILD_DIR)/nitride_study_render" "$(BUILD_DIR)/sound-studies-05" --study-05

clean:
	rm -rf "$(BUILD_DIR)"
