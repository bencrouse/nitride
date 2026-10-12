#include "Fm.h"

namespace Nitride::DSP
{
FmTables::FmTables()
{
    constexpr int points=512;
    // Fourier coefficients of exp(i * index * waveform(p)). Odd, half-wave
    // symmetric waveforms have real coefficients and c[-n]=(-1)^n c[n].
    for(size_t shape=0;shape<table.size();++shape)for(size_t a=0;a<17;++a)
    {
        const auto harmonics=fmHarmonics(static_cast<FmShape>(shape),static_cast<double>(a)/16);
        std::array<double,points> waves{};
        for(int p=0;p<points;++p)waves[static_cast<size_t>(p)]=fmWave(2*std::numbers::pi*p/points,harmonics);
        for(size_t index=0;index<129;++index)
        {
            auto& coefficients=table[shape][a][index];
            for(int p=0;p<points;++p)
            {
                const auto phase=2*std::numbers::pi*p/points;
                const auto real=std::cos(static_cast<double>(index)/32*waves[static_cast<size_t>(p)]);
                const auto imaginary=std::sin(static_cast<double>(index)/32*waves[static_cast<size_t>(p)]);
                const auto stepCos=std::cos(phase),stepSin=std::sin(phase);double cosine=1,sine=0;
                for(auto& coefficient:coefficients)
                {
                    coefficient+=(real*cosine+imaginary*sine)/points;
                    const auto nextCos=cosine*stepCos-sine*stepSin;sine=sine*stepCos+cosine*stepSin;cosine=nextCos;
                }
            }
        }
    }
}
FmCoefficients FmTables::coefficients(FmShape shape,double amount,double index) const noexcept
{
    const auto a=std::clamp(amount,0.0,1.0)*16,b=std::clamp(index,0.0,4.0)*32;
    const auto ai=std::min(static_cast<size_t>(a),size_t{15}),bi=std::min(static_cast<size_t>(b),size_t{127});
    const auto af=a-static_cast<double>(ai),bf=b-static_cast<double>(bi);FmCoefficients result{};
    const auto& t=table[static_cast<size_t>(shape)];
    for(size_t n=0;n<result.size();++n)result[n]=(1-af)*((1-bf)*t[ai][bi][n]+bf*t[ai][bi+1][n])+af*((1-bf)*t[ai+1][bi][n]+bf*t[ai+1][bi+1][n]);
    return result;
}
const FmTables& fmTables(){static const FmTables tables;return tables;}
}
