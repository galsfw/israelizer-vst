# SaturAir

Esqueleto de um plugin VST3 de saturação/exciter em **C++ + JUCE 8**, com a mesma
lógica do **Fresh Air** (Slate Digital): um exciter multibanda que gera harmônicos
apenas nas bandas de cima e os soma ao sinal original.

```
                       ┌── LP 2.5 kHz ──────────────────────────── atraso(L) ──┐
entrada ──┬────────────┤                                                         │
          │            └── (x - low) ──┬── LP 8 kHz ── mid  ── sat ── "High" ──►(+)──► gain ── saída
          │                           │                                          ▲
          │                           └── resto ────── high ── sat ── "Air"  ────┘
          │
          └── L = latência dos sobreamostradores (compensada no caminho seco)
```

- **Divisão em bandas**: cortes Linkwitz-Riley de 4ª ordem (2 cascatas de
  Butterworth). A soma é exata por construção: `low + mid + high == entrada`.
- **Saturação aditiva** (não substitui o sinal, soma em cima dele):
  `y = x + amount * tanh(drive * x) / drive` — gera os harmônicos de "brilho/ar".
- **Sobreamostragem 4x** (`juce::dsp::Oversampling`, IIR meia-band) nos dois
  saturadores para não aliasar.
- **Compensação de latência**: a banda grave (que não passa pelos saturadores) é
  atrasada pela latência exata do sobreamostrador e a latência é reportada ao DAW
  com `setLatencySamples()` — as bandas chegam sempre alinhadas.

## Parâmetros

| Param ID   | Nome     | Faixa        | O que faz                                          |
|------------|----------|--------------|----------------------------------------------------|
| `high`     | High     | 0–100 %      | Excita a banda 2.5–8 kHz (presença/brilho)         |
| `air`      | Air      | 0–100 %      | Excita a banda > 8 kHz (ar/agudeza)                |
| `output`   | Output   | −24…+24 dB   | Ganho de saída (suavizado)                          |
| `lowMidHz` | Low/Mid  | 100–16000 Hz | 1º crossover (default 2500 Hz)                     |
| `midHighHz`| Mid/High | 100–16000 Hz | 2º crossover (default 8000 Hz)                     |

## Estrutura

```
SaturAir/
├── CMakeLists.txt                  # FetchContent do JUCE (ou -DJUCE_PATH=...)
├── .github/workflows/build-vst3.yml# CI: configura, compila e publica o VST3
└── Source/
    ├── PluginProcessor.h/.cpp      # APVTS, processBlock, estado/latência
    ├── PluginEditor.h/.cpp         # 3 knobs + 2 crossovers (attachments)
    └── Dsp/
        ├── Biquad.h                # biquad RBJ (low-pass) -> LR4
        ├── BandSplitter.h          # entrada -> low / mid / high
        ├── SaturationStage.h       # sobreamostrador + tanh aditivo
        └── SampleDelay.h           # compensação de latência da banda grave
```

## Build

### Local (Windows)

```powershell
cmake -B build -A x64
cmake --build build --config Release --parallel
```

O VST3 sai em `build/SaturAir_artefacts/Release/VST3/SaturAir.vst3`.
Na primeira configuração o CMake baixa o JUCE (tag `8.0.14`) via FetchContent
(e usa o gerador Visual Studio instalado; se preferir, adicione
`-G "Visual Studio 17 2022"`).

Se já tiver o JUCE instalado: `cmake -B build -DJUCE_PATH="C:/JUCE"`.

### GitHub Actions

O workflow `.github/workflows/build-vst3.yml` já está pronto: a cada push ele
configura, compila em Release e publica o VST3 como *artifact*
(`SaturAir-Windows-VST3`). Baixe na aba **Actions → Build VST3 → Artifacts**.

## Onde ajustar (os pontos de "gosto")

- **Drive de cada banda**: `midDrive` / `airDrive` em `PluginProcessor.h`
  (padrão 6.0 e 4.0). Maior = mais harmônicos, mais agressivo.
- **Fórmula da saturação**: `SaturationStage::process()` — troque `tanh` por
  `x / (1 + |x|)`, polinômio de 3ª ordem, etc.
- **Caráter do corte**: `Biquad::setLowPass()` (Q, ordem) em `BandSplitter.h`.
- **Qualidade/fator do sobreamostrador**: 2º argumento do construtor de
  `juce::dsp::Oversampling` em `SaturationStage::prepare()` (2 = 4x, 3 = 8x).
- **Suavização**: `SmoothedValue` em `SaturationStage` (amount/drive) e
  `outputGain` no processador.

## TODOs (para sair do esqueleto)

- Medidores de entrada/saída e botão de bypass por banda.
- Fator de sobreamostragem como parâmetro (1x/2x/4x/8x) — exige recriar o
  `Oversampling` fora do thread de áudio.
- Modo paralelo/seco-úmido global e presets de fábrica.
- Oversampling do caminho "dry" em vez do atraso inteiro (evita o corte de
  fase do IIR meia-band perto de Nyquist na banda grave — hoje já é
  compensado no tempo, mas o atraso inteiro é a solução mais simples).
- Presets e automação de crossovers com resposta visual no editor.

## Licença

O código do plugin é seu. O JUCE é distribuído sob **EULA do JUCE / AGPLv3** —
se for distribuir comercialmente sem as cláusulas da AGPL, adquira a licença
comercial do JUCE. Mantenha o splash screen do JUCE ativo, a menos que sua
licença permita removê-lo.
