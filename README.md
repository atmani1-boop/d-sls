# 🌟 Atmani Smart Lighting Node — Firmware ESP32-C6

Firmware professionnel pour contrôleur d'éclairage intelligent basé sur **ESP32-C6** avec support : 
- **Thread IPv6 Mesh** (FTD + Border Router)
- **RDMnet (E1.33)** — Configuration, sensors, logs
- **sACN (E1.31)** — DMX sur IP multicast
- **224 canaux DMX → 56 fixtures RGBW**
- **Gestion énergie** (INA226, protections, emergency dimming)
- **16 scènes internes** avec failover
- **Synchronisation mesh** inter-nœuds

---

## 🚀 Démarrage Rapide

### Prérequis
- [ESP-IDF v5.1+](https://docs.espressif.com/projects/esp-idf/en/latest/esp32c6/get-started/)
- ESP32-C6 DevKit
- Câble USB-C

### Compilation

```bash
# Cloner le projet
git clone https://github.com/atmani1-boop/d-sls.git
cd d-sls

# Configurer ESP-IDF
. $HOME/esp/esp-idf/export.sh

# Compiler
idf.py set-target esp32c6
idf.py build

# Le fichier .bin sera dans : build/atmani-lighting.bin
```

### Flashage

```bash
# Flash complet (firmware + partitions)
idf.py -p /dev/ttyUSB0 flash

# Monitoring série
idf.py -p /dev/ttyUSB0 monitor
```

---

## 📋 Fonctionnalités

### ✅ Réseau
- Thread 1.3 FTD (Full Thread Device)
- IPv6 routing + auto-healing
- Border Router → LAN
- Mesh synchronization

### ✅ Protocoles Lumière
- **RDMnet (E1.33)** : configuration distante, sensors, identify
- **sACN (E1.31)** : streaming DMX sur IPv6 multicast
- Failover automatique scène interne

### ✅ Moteur Lumière
- 224 canaux DMX → 56 fixtures RGBW
- PWM haute résolution (ESP32-C6 LEDC)
- Correction gamma
- Intensité globale
- Emergency dimming

### ✅ Gestion Énergie
- Monitoring INA226 (tension, courant, puissance)
- Protections : overcurrent, overpower, undervoltage
- Emergency dimming automatique
- Reporting RDMnet sensors

### ✅ Scènes
- 16 scènes programmables en NVS
- Fade/transitions
- Fallback si perte sACN
- Sync mesh inter-nœuds

### ✅ Configuration
- DIP Switch AUTO/MANUEL
- Profiles prédéfinis
- Override RDMnet
- Stockage NVS

### ✅ Interface
- LED RGB état système
- CLI UART (commandes debug)
- Web UI (optionnel)

---

## 📐 Architecture

Voir **[docs/ARCHITECTURE.md](docs/ARCHITECTURE.md)** pour le dossier technique complet.

---

## 🔧 Configuration

### DIP Switch (exemple 4 bits)
| DIP | Mode | Univers | Start Addr | Scène |
|-----|------|---------|------------|-------|
| 0000 | AUTO | 1 | 1 | 0 |
| 0001 | MANUEL | 1 | 1 | 1 |
| 0010 | MANUEL | 1 | 57 | 0 |

### Limites Énergie (NVS)
```c
max_current_ma = 20000   // 20A
max_power_w = 4800       // 4.8kW
min_voltage_v = 22.0     // Sous-tension
```

---

## 📚 Documentation

- [ARCHITECTURE.md](docs/ARCHITECTURE.md) — Dossier technique complet
- [BUILD_GUIDE.md](docs/BUILD_GUIDE.md) — Compilation détaillée
- [FLASH_GUIDE.md](docs/FLASH_GUIDE.md) — Flashage et debugging

---

## 📊 Performances

- **Latence sACN → PWM** : < 5ms
- **Refresh rate PWM** : 1kHz
- **Thread mesh healing** : < 2s
- **RDMnet response** : < 50ms

---

## 🐛 Debug

```bash
# Logs détaillés
idf.py menuconfig
# → Component config → Log output → Verbose

# Monitor série
idf.py monitor

# CLI UART
minicom -D /dev/ttyUSB0 -b 115200
> help
> status
> scenes list
> energy status
```

---

## 📄 Licence

Propriétaire — Atmani Smart Lighting  
© 2026 Tous droits réservés

---

**Fait avec ❤️ pour l'éclairage professionnel intelligent**