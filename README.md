# Lector RFID ISO15693 con PN5180 y BLE

Firmware ESP-IDF para un lector RFID ISO15693 basado en ESP32 y PN5180. El equipo lee el UID del TAG RFID, lo muestra por monitor serial, lo publica por BLE para verlo desde nRF Connect y usa un LED de estado conectado al GPIO14.

## Estado actual

- Lectura RFID ISO15693 con PN5180 por SPI.
- Modo LPCD del PN5180 para esperar deteccion con menor consumo cuando no hay TAG.
- Deteccion de UID del TAG en el monitor serial.
- Publicacion BLE por advertising usando NimBLE.
- Visualizacion desde nRF Connect sin necesidad de conectar por GATT.
- LED de estado en GPIO14:
  - LED activo en LOW.
  - Apagado cuando no hay TAG.
  - Parpadea cada 200 ms cuando hay TAG detectado.
- Escaneo I2C disponible solo como diagnostico inicial.

## Hardware principal

### PN5180 por SPI

| Senal | GPIO ESP32 |
| --- | --- |
| MOSI | GPIO23 |
| MISO | GPIO19 |
| SCLK | GPIO18 |
| CS | GPIO5 |
| BUSY | GPIO4 |
| RST | GPIO2 |
| IRQ | GPIO27 |

### I2C

| Senal | GPIO ESP32 |
| --- | --- |
| SDA | GPIO21 |
| SCL | GPIO22 |

### Otros pines

| Funcion | GPIO ESP32 | Nota |
| --- | --- | --- |
| Enable TPS61022 5V | GPIO12 | Alimenta el PN5180 |
| LED estado | GPIO14 | Activo en LOW |

## Funcionamiento

Al iniciar, el firmware:

1. Inicializa el bus I2C.
2. Habilita el TPS61022 para alimentar el PN5180.
3. Inicializa BLE/NimBLE.
4. Escanea dispositivos I2C como diagnostico.
5. Inicializa SPI y PN5180.
6. Lee la version de firmware del PN5180.
7. Ejecuta inventario ISO15693.
8. Si no encuentra TAG durante varios intentos, entra a LPCD.
9. Cuando el PN5180 activa IRQ, vuelve a inventario normal para confirmar el UID.

Ejemplo de UID detectado:

```text
TAG ISO15693 detectado. UID: 5E 4E 0F 1E 53 01 04 E0
```

## BLE con nRF Connect

El firmware anuncia por BLE el estado del lector:

- Sin TAG: `RFID SIN TAG`
- Con TAG: `RFID 5E4E0F1E530104E0`

Tambien publica el UID en Manufacturer Data:

```text
FF FF 5E 4E 0F 1E 53 01 04 E0
```

Para verificar desde el telefono:

1. Abrir nRF Connect.
2. Entrar a `Scanner`.
3. Buscar `RFID SIN TAG` o `RFID <UID>`.
4. No es necesario presionar `CONNECT`.
5. Al acercar o retirar el TAG, el nombre anunciado cambia.

## LPCD del PN5180

El modo LPCD usa el pin `IRQ` del PN5180 en `GPIO27`.

Flujo actual:

1. El lector intenta inventario ISO15693 normal.
2. Si no detecta TAG por varios intentos, publica `RFID SIN TAG BAT:x%`.
3. Apaga el campo RF y manda al PN5180 a `SWITCH_MODE LPCD`.
4. La tarea RF queda esperando interrupcion por `GPIO27`.
5. Al despertar por IRQ, lee `IRQ_STATUS`, limpia la IRQ y vuelve a intentar inventario ISO15693.

Como el nivel activo del pin IRQ puede variar segun modulo/configuracion, el GPIO esta configurado por cualquier flanco. El inventario posterior valida si realmente hay TAG.

## Estructura del proyecto

```text
main/
  main.c
  app/
    ble/
      ble_advertiser.c
      ble_advertiser.h
    rf/
      lector_rf.c
      lector_rf.h
    sources/
      TPS61022_+5V.c
      TPS61022_+5V.h
    status_led/
      status_led.c
      status_led.h
components/
  drivers/
    i2c_manager/
    spi_manager/
  sensors/
    pn5180/
    bmi088/
partitions/
  partitions.csv
```

## Compilacion

Proyecto probado con ESP-IDF v5.5.4 y target `esp32`.

```bash
idf.py set-target esp32
idf.py build
```

Si se usa CMake directamente en este entorno:

```powershell
$env:IDF_PATH='C:/esp/v5.5.4/esp-idf'
cmake --build build
```

## Flash y monitor

```bash
idf.py flash monitor
```

Si el puerto no se detecta automaticamente:

```bash
idf.py -p COMx flash monitor
```

Reemplazar `COMx` por el puerto real del ESP32.

## Notas

- Los dispositivos I2C encontrados actualmente se usan solo como diagnostico.
- El objetivo principal actual es confirmar deteccion RFID sin PC, usando BLE y el LED de estado.
- Algunos pines usados, como GPIO2, GPIO5 y GPIO12, pueden afectar el arranque del ESP32 si el hardware externo los fuerza durante reset. Revisar el esquematico si aparecen arranques inestables.
