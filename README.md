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
| `shape`    | Shape    | 6 opções     | Forma de onda dos saturadores: Tanh, ArcTan, Cubic, Rational, Valve, Hard (troca com morph de 10 ms) |

## Estrutura

```
SaturAir/
├── CMakeLists.txt                  # FetchContent do JUCE + módulo sound_meter
├── .github/workflows/build-vst3.yml# CI: build + ctest + pluginval + artefato
├── Tests/
│   ├── CMakeLists.txt              # alvo SaturAirTests (Catch2, opcional)
│   └── SaturAirTests.cpp           # testes do DSP (divisor, latência, formas)
└── Source/
    ├── PluginProcessor.h/.cpp      # APVTS, processBlock, estado/latência, picos
    ├── PluginEditor.h/.cpp         # knobs + crossovers + shape + medidores
    └── Dsp/
        ├── Biquad.h                # biquad RBJ (low-pass) -> LR4
        ├── BandSplitter.h          # entrada -> low / mid / high
        ├── SaturationStage.h       # sobreamostrador + 6 waveshapers
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
configura, compila em Release, roda os **testes unitários** (`ctest`), valida o
plugin com **pluginval** (strictness 10) e publica o VST3 como *artifact*
(`SaturAir-Windows-VST3`). Baixe na aba **Actions → Build VST3 → Artifacts**.

Ao criar uma tag `v*` (`git tag v0.2.0 && git push origin v0.2.0`), um job
extra publica uma **Release** com o zip do VST3 anexado.

### Testes unitários

```powershell
cmake -B build -A x64 -DSATURAIR_BUILD_TESTS=ON
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```

A suíte (`Tests/SaturAirTests.cpp`, Catch2) cobre as garantias centrais:
complementaridade do divisor (`low + mid + high == entrada`), separação real
das bandas, atraso exato do `SampleDelay`, latência reportada vs. atraso real
do sobreamostrador, passthrough com `amount = 0` e as 6 formas de onda
(ganho ~1 para sinais fracos, saída limitada, assimetria do Valve).

## Onde ajustar (os pontos de "gosto")

- **Drive de cada banda**: `midDrive` / `airDrive` em `PluginProcessor.h`
  (padrão 6.0 e 4.0). Maior = mais harmônicos, mais agressivo.
- **Fórmula da saturação**: `SaturationStage::applyShape()` — as 6 formas já
  estão lá (`tanh`, `arctan`, `cubic`, `rational`, `valve`, `hard`); adicionar
  uma nova é um case no `switch` + uma entrada no ComboBox/`AudioParameterChoice`.
- **Caráter do corte**: `Biquad::setLowPass()` (Q, ordem) em `BandSplitter.h`.
- **Qualidade/fator do sobreamostrador**: 2º argumento do construtor de
  `juce::dsp::Oversampling` em `SaturationStage::prepare()` (2 = 4x, 3 = 8x).
- **Suavização**: `SmoothedValue` em `SaturationStage` (amount/drive) e
  `outputGain` no processador.

## TODOs (para sair do esqueleto)

- [x] Medidores de entrada/saída (sound_meter no editor, picos atômicos).
- [x] Testes unitários (Catch2) e validação pluginval no CI.
- [x] Formas de onda alternativas (parâmetro `Shape`: 6 waveshapers).
- Bypass por banda (só a mid ou só a high).
- Fator de sobreamostragem como parâmetro (1x/2x/4x/8x) — exige recriar o
  `Oversampling` fora do thread de áudio.
- Modo paralelo/seco-úmido global e presets de fábrica.
- Oversampling do caminho "dry" em vez do atraso inteiro (evita o corte de
  fase do IIR meia-band perto de Nyquist na banda grave — hoje já é
  compensado no tempo, mas o atraso inteiro é a solução mais simples).
- Presets e automação de crossovers com resposta visual no editor.

## Referências (awesome-juce)

Ideias levantadas na lista [sudara/awesome-juce](https://github.com/sudara/awesome-juce)
que cabem neste plugin (ordem de interesse):

- **[melatonin_test_helpers](https://github.com/sudara/melatonin_test_helpers)** —
  matchers de `AudioBlock` para testar a propriedade `low + mid + high == entrada`
  com menos boilerplate.
- **[fontaudio](https://github.com/fefanto/fontaudio)** — ícones para a UI
  (bypass, power) quando formos sair do esqueleto.
- **[melatonin_blur](https://github.com/sudara/melatonin_blur)** — sombras/blur
  rápidos para dar acabamento na UI escura.
- **[sst-waveshapers](https://github.com/surge-synthesizer/sst-waveshapers)** (GPL-3,
  compatível com a licença deste repo) — formas de onda do Surge, se as 6
  atuais não bastarem.
- **[chowdsp_utils](https://github.com/Chowdhury-DSP/chowdsp_utils)** — utilitários
  (presets/json) quando formos implementar presets.
- **[clap-juce-extensions](https://github.com/free-audio/clap-juce-extensions)** —
  gerar também o formato CLAP além do VST3.
- **[pamplejuce](https://github.com/sudara/pamplejuce)** — referência de estrutura
  de CI/testes (já seguimos o padrão dele: Catch2 + pluginval no Actions).

## Licença

O código do plugin é seu. O JUCE é distribuído sob **EULA do JUCE / AGPLv3** —
se for distribuir comercialmente sem as cláusulas da AGPL, adquira a licença
comercial do JUCE. Mantenha o splash screen do JUCE ativo, a menos que sua
licença permita removê-lo.
