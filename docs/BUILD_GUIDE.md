# 🔨 Build Guide - Atmani Smart Lighting Node

## Prérequis

### 1. ESP-IDF v5.1+

Le firmware nécessite **ESP-IDF v5.1** ou supérieur.

#### Installation ESP-IDF

```bash
# Cloner ESP-IDF
mkdir -p ~/esp
cd ~/esp
git clone --recursive https://github.com/espressif/esp-idf.git
cd esp-idf
git checkout v5.1

# Installer les dépendances
./install.sh esp32c6

# Activer l'environnement (à faire dans chaque nouveau terminal)
. $HOME/esp/esp-idf/export.sh
```

**Vérification** :
```bash
idf.py --version
# Devrait afficher : ESP-IDF v5.1.x
```

### 2. Matériel

- **ESP32-C6 DevKit** ou compatible
- Câble **USB-C** pour programmation
- Alimentation 24V DC pour système complet

---

## 📥 Récupération du Code

```bash
# Cloner le repository
git clone https://github.com/atmani1-boop/d-sls.git
cd d-sls
```

---

## ⚙️ Configuration

### Configuration automatique (Recommandé)

Le fichier `sdkconfig.defaults` contient la configuration par défaut optimale :

```bash
# Simplement définir la cible
idf.py set-target esp32c6
```

### Configuration manuelle (Optionnelle)

Pour personnaliser la configuration :

```bash
idf.py menuconfig
```

**Paramètres importants** :

- **Component config → OpenThread**
  - Enable OpenThread
  - FTD (Full Thread Device)
  - Border Router

- **Component config → LWIP**
  - Enable IPv6
  - Enable IPv6 autoconfig

- **Component config → Log output**
  - Default level: Info
  - Enable colors

---

## 🔨 Compilation

### Compilation standard

```bash
# Activer ESP-IDF
. $HOME/esp/esp-idf/export.sh

# Compiler
idf.py build
```

**Sortie attendue** :
```
[100%] Linking CXX executable atmani-smart-lighting.elf
[100%] Built target atmani-smart-lighting.elf
```

### Localisation du firmware

Le firmware compilé se trouve dans :
```
build/atmani-smart-lighting.bin
```

Autres fichiers générés :
- `build/atmani-smart-lighting.elf` - Fichier ELF avec symboles debug
- `build/bootloader/bootloader.bin` - Bootloader
- `build/partition_table/partition-table.bin` - Table de partitions

---

## 📊 Analyse de la Taille

Afficher l'utilisation mémoire :

```bash
idf.py size

# ou pour plus de détails
idf.py size-components
idf.py size-files
```

**Exemple de sortie** :
```
Total sizes:
 DRAM .data size:    12345 bytes
 DRAM .bss  size:    23456 bytes
 Used static DRAM:   35801 bytes ( 144935 available, 19.8% used)
 Used static IRAM:   89012 bytes (  42076 available, 67.9% used)
 Flash code:    234567 bytes
 Flash rodata:   45678 bytes
 Total image size: ~345678 bytes (.bin may be padded larger)
```

---

## 🧹 Nettoyage

### Nettoyage partiel

```bash
# Nettoie les fichiers de build
idf.py clean
```

### Nettoyage complet

```bash
# Nettoie tout y compris la configuration
idf.py fullclean

# Reconfigurer après fullclean
idf.py set-target esp32c6
```

---

## 🐛 Résolution de Problèmes

### Erreur : "OpenThread not found"

**Solution** :
```bash
# Vérifier ESP-IDF v5.1+
idf.py --version

# Si version < 5.1, mettre à jour
cd ~/esp/esp-idf
git checkout v5.1
git pull
./install.sh esp32c6
```

### Erreur : "Target not set"

**Solution** :
```bash
idf.py set-target esp32c6
```

### Erreur : "Component not found"

**Solution** :
```bash
# Nettoyer et reconfigurer
idf.py fullclean
idf.py set-target esp32c6
idf.py build
```

### Warnings LEDC/GPIO

Les warnings concernant les GPIO non connectés sont normaux si vous n'avez pas tout le matériel connecté. Le firmware démarre quand même.

---

## 🚀 Compilation Rapide (Script)

Utiliser le script fourni :

```bash
./scripts/build.sh
```

Ce script :
1. Active ESP-IDF
2. Configure la cible ESP32-C6
3. Compile
4. Affiche la taille

---

## 📝 Notes Importantes

### Dépendances

Le projet utilise les composants ESP-IDF suivants :
- `nvs_flash` - Stockage non-volatile
- `esp_netif` - Interface réseau
- `driver` - Drivers matériels (GPIO, I2C, LEDC, UART)
- `openthread` - Stack Thread mesh
- `lwip` - Stack TCP/IP

Tous sont inclus dans ESP-IDF v5.1+.

### Partitions

Table de partitions par défaut ESP-IDF :
- Bootloader : 0x1000
- Partition table : 0x8000
- NVS : 0x9000 (24KB)
- PHY_init : 0xF000 (4KB)
- Factory app : 0x10000 (1MB+)

Pour une table personnalisée, créer `partitions.csv`.

### Optimisation

Pour un build de production :

```bash
idf.py menuconfig
# → Compiler options → Optimization Level → Release (-Os)
```

---

## ✅ Checklist de Compilation

- [ ] ESP-IDF v5.1+ installé
- [ ] Environnement activé (`. export.sh`)
- [ ] Cible ESP32-C6 configurée
- [ ] `idf.py build` réussi
- [ ] Fichier `.bin` généré dans `build/`
- [ ] Taille firmware < 2MB (pour OTA futur)

---

**Prochaine étape** : [FLASH_GUIDE.md](FLASH_GUIDE.md) pour flasher le firmware sur l'ESP32-C6.
