# Gym Counter — BitDogLab (RP2040)

Contador de séries e repetições para academia, feito em C com o Pico SDK.

## Hardware

| Componente | Pino BitDogLab |
|------------|---------------|
| Botão A    | GP5           |
| Botão B    | GP6           |
| OLED SDA   | GP14          |
| OLED SCL   | GP15          |
| OLED I2C   | i2c1 / 0x3C   |
| Display    | SSD1306 128x64|

---

## Como usar o app

### Tela de Configuração
- **Botão A** → incrementa o valor selecionado (séries ou reps)
- **Botão B** → alterna entre "Séries" e "Reps" / no campo Reps confirma e inicia

### Tela de Contagem
- **Botão A** → conta +1 repetição
- **Botão B** → encerra a série atual e vai para o resumo

### Tela de Série Concluída
- **Botão A** → inicia a próxima série
- **Botão B** → encerra o treino antecipado

### Tela de Treino Completo
- **Botão A ou B** → volta à configuração para novo treino

---

## Build

### Pré-requisitos
- [Pico SDK](https://github.com/raspberrypi/pico-sdk) instalado
- CMake >= 3.13
- arm-none-eabi-gcc

### Compilar

```bash
export PICO_SDK_PATH=/caminho/para/pico-sdk

mkdir build && cd build
cmake ..
make -j4
```

O arquivo gerado será `build/gym_counter.uf2`.

### Gravar na BitDogLab

1. Segure o botão **BOOTSEL** da placa
2. Conecte o USB
3. Solte o BOOTSEL — a placa aparece como pendrive `RPI-RP2`
4. Copie o arquivo `gym_counter.uf2` para o pendrive
5. A placa reinicia automaticamente e o app começa

---

## Estrutura do projeto

```
gym_counter/
├── CMakeLists.txt
├── README.md
├── include/
│   └── ssd1306.h       # Driver do display OLED
└── src/
    ├── main.c          # Lógica principal (máquina de estados)
    └── ssd1306.c       # Implementação do driver + fonte bitmap
```

---

## Arquitetura do código

O app usa uma **máquina de estados** com 4 estados:

```
CONFIG → COUNTING → SERIE_DONE → COUNTING (próxima série)
                              └→ WORKOUT_DONE → CONFIG
```

- `STATE_CONFIG` — usuário configura séries e reps
- `STATE_COUNTING` — contagem ativa, exibe número grande no display
- `STATE_SERIE_DONE` — resumo da série, aguarda confirmação
- `STATE_WORKOUT_DONE` — parabéns, exibe total de reps
