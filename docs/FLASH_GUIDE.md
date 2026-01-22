# 📲 Flash Guide - Atmani Smart Lighting Node

## Prérequis

- Firmware compilé (voir [BUILD_GUIDE.md](BUILD_GUIDE.md))
- ESP32-C6 DevKit connecté via USB-C
- ESP-IDF activé dans le terminal

---

## 🔌 Connexion Hardware

### 1. Connecter l'ESP32-C6

1. Brancher le câble **USB-C** entre l'ESP32-C6 et votre PC
2. Le port série devrait apparaître automatiquement

### 2. Identifier le Port Série

**Linux** :
```bash
ls /dev/ttyUSB*
# ou
ls /dev/ttyACM*

# Exemple : /dev/ttyUSB0
```

**macOS** :
```bash
ls /dev/cu.usbserial*
# ou
ls /dev/cu.usbmodem*

# Exemple : /dev/cu.usbserial-0001
```

**Windows** :
```cmd
# Ouvrir le Gestionnaire de périphériques
# → Ports (COM & LPT)
# Exemple : COM3
```

---

## ⚡ Flashage

### Méthode 1 : Flash Complet (Recommandé)

Flash le firmware, le bootloader et les partitions :

```bash
# Activer ESP-IDF
. $HOME/esp/esp-idf/export.sh

# Naviguer vers le projet
cd d-sls

# Flasher (remplacer /dev/ttyUSB0 par votre port)
idf.py -p /dev/ttyUSB0 flash
```

**Processus** :
1. Détection automatique de l'ESP32-C6
2. Flash du bootloader (0x0)
3. Flash de la table de partitions (0x8000)
4. Flash de l'application (0x10000)
5. Vérification

**Sortie attendue** :
```
Detecting chip type... ESP32-C6
Chip is ESP32-C6 (revision vX.Y)
...
Hash of data verified.
Leaving...
Hard resetting via RTS pin...
```

### Méthode 2 : Flash Application Seulement

Pour reflasher uniquement l'application (plus rapide) :

```bash
idf.py -p /dev/ttyUSB0 app-flash
```

### Méthode 3 : Flash + Monitor

Flash et lance immédiatement le monitoring série :

```bash
idf.py -p /dev/ttyUSB0 flash monitor
```

Pour quitter le monitor : `Ctrl+]`

---

## 📊 Monitoring Série

### Lancer le Monitor

```bash
idf.py -p /dev/ttyUSB0 monitor
```

**Raccourcis utiles** :
- `Ctrl+]` - Quitter
- `Ctrl+T` puis `Ctrl+H` - Aide
- `Ctrl+T` puis `Ctrl+R` - Reset ESP32
- `Ctrl+T` puis `Ctrl+P` - Pause/Resume

### Sortie Attendue au Démarrage

```
I (123) MAIN: ========================================
I (124) MAIN:   Atmani Smart Lighting Node
I (125) MAIN:   ESP32-C6 Firmware v1.0.0
I (126) MAIN:   Build: Jan 22 2026 13:05:00
I (127) MAIN: ========================================
I (128) LOGGER: Logger initialized - Level: INFO, Colors: ENABLED
I (200) CONFIG: Initializing configuration manager
I (201) CONFIG: DIP switch value: 0x00
I (202) CONFIG: Applied DIP profile 0x00: Mode=AUTO, U=1, Addr=1, Scene=0
I (300) THREAD: Initializing Thread network
I (301) THREAD: OpenThread initialized
I (302) THREAD: Thread task created
...
I (1500) MAIN: ========================================
I (1501) MAIN: Atmani Smart Lighting Node - READY
I (1502) MAIN: ========================================
```

### CLI UART Interactif

Dans le monitor, taper des commandes :

```
> help
Available commands:
  help     - Show this help
  status   - Show system status
  version  - Show firmware version

> status
System Status:
  State: NORMAL
  TODO: Add more status info

> version
Firmware Version: 1.0.0
Build Date: Jan 22 2026 13:05:00
```

---

## 🧹 Effacement Flash

### Effacer Complètement la Flash

Pour un reset complet (efface NVS, configuration, etc.) :

```bash
idf.py -p /dev/ttyUSB0 erase-flash
```

⚠️ **Attention** : Efface **toutes les données** incluant :
- Configuration NVS
- Scènes sauvegardées
- Calibrations
- Historique

Après effacement, reflasher le firmware.

---

## 🐛 Résolution de Problèmes

### Erreur : "Port not found"

**Symptômes** :
```
serial.serialutil.SerialException: [Errno 2] could not open port /dev/ttyUSB0
```

**Solutions** :
1. Vérifier la connexion USB-C
2. Identifier le bon port : `ls /dev/tty*`
3. Vérifier les permissions (Linux) :
   ```bash
   sudo usermod -a -G dialout $USER
   # Se déconnecter/reconnecter
   ```

### Erreur : "Failed to connect"

**Symptômes** :
```
A fatal error occurred: Failed to connect to ESP32-C6
```

**Solutions** :
1. Maintenir le bouton **BOOT** pendant le flash
2. Débrancher/rebrancher l'USB
3. Essayer un autre câble USB-C
4. Vérifier que le câble supporte les données (pas que charge)

### Erreur : "Chip not supported"

**Symptômes** :
```
A fatal error occurred: This chip is not supported
```

**Solutions** :
1. Vérifier ESP-IDF v5.1+ : `idf.py --version`
2. Re-installer ESP32-C6 support :
   ```bash
   cd ~/esp/esp-idf
   ./install.sh esp32c6
   ```

### Bootloop / Crash au Démarrage

**Symptômes** :
```
Guru Meditation Error: Core 0 panic'ed (LoadProhibited)
```

**Solutions** :
1. Effacer la flash : `idf.py erase-flash`
2. Reflasher : `idf.py flash`
3. Vérifier les logs pour l'erreur spécifique
4. Désactiver temporairement les GPIO non connectés

---

## 🚀 Flash Rapide (Script)

Utiliser le script fourni :

```bash
./scripts/flash.sh [PORT]

# Exemples :
./scripts/flash.sh /dev/ttyUSB0
./scripts/flash.sh /dev/ttyACM0
./scripts/flash.sh COM3
```

Le script :
1. Active ESP-IDF
2. Flash le firmware
3. Lance le monitor

---

## 📝 Notes Importantes

### Permissions Linux

Ajouter votre utilisateur au groupe `dialout` :

```bash
sudo usermod -a -G dialout $USER
```

Se déconnecter/reconnecter pour appliquer.

### Port Série macOS

Sur macOS, privilégier les ports `cu.*` plutôt que `tty.*` :

```bash
# Bon
idf.py -p /dev/cu.usbserial-0001 flash

# Éviter
idf.py -p /dev/tty.usbserial-0001 flash
```

### Flash OTA (Futur)

Le firmware supporte OTA (Over-The-Air) update. À implémenter :

1. Configurer table de partitions avec `ota_0` et `ota_1`
2. Implémenter serveur HTTP/HTTPS
3. Utiliser ESP-IDF OTA API
4. Flash initial via USB, updates suivantes via WiFi/Thread

---

## ✅ Checklist de Flashage

- [ ] ESP32-C6 connecté via USB-C
- [ ] Port série identifié
- [ ] Permissions OK (Linux/macOS)
- [ ] Firmware compilé présent dans `build/`
- [ ] `idf.py flash` réussi
- [ ] Monitor série affiche logs démarrage
- [ ] LED RGB s'allume (vert = NORMAL)
- [ ] CLI UART répond aux commandes

---

## 🔄 Workflow de Développement

Pour itérer rapidement :

```bash
# Terminal 1 : Compilation continue
idf.py build

# Terminal 2 : Flash + monitor
idf.py -p /dev/ttyUSB0 flash monitor

# Ou en une commande
idf.py -p /dev/ttyUSB0 build flash monitor
```

---

**Le système est maintenant flashé et opérationnel ! 🎉**

Voir [ARCHITECTURE.md](ARCHITECTURE.md) pour comprendre le fonctionnement interne.
