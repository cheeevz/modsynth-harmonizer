# ModSynth Harmonizer

**Module Eurorack de génération d'accords en temps réel sur STM32.**
À partir d'un signal monophonique (un VCO, un synthé), le module génère jusqu'à trois voix supplémentaires qui forment un accord. L'accord se choisit sur écran tactile et se pilote en tension de contrôle (CV) et en gate depuis un système modulaire.

> 🚧 Projet en cours. Voir la [feuille de route](#feuille-de-route).

---

## L'histoire du projet

En 2019-2020, en dernière année d'école d'ingénieur (Polytech Sorbonne, Électronique et Informatique), j'ai conçu avec trois camarades **ModSynth**, un synthétiseur modulaire au format Eurorack : séquenceur MIDI, VCO, VCF, VCA, LFO, ADSR, alimentation et un module d'effets numériques.

J'étais responsable du **module DSP**, sur STM32F4. Sa fonction la plus ambitieuse : transformer un signal monophonique en accord. La méthode était la suivante :

1. mesurer la période du signal à partir de ses maximums de tension ;
2. capturer un cycle dans un tableau ;
3. relire ce cycle à plusieurs vitesses pour obtenir les autres notes (harmoniques 3 et 5).

Le module fonctionnait, mais mal :

- la détection de période était sensible au bruit, donc la hauteur des voix tremblait ;
- un clic apparaissait à chaque répétition du cycle ;
- seuls des rapports entiers étaient possibles, donc impossible de choisir le type d'accord ;
- les exigences n'étaient pas chiffrées : la validation se faisait à l'œil, à l'oscilloscope.

**Ce dépôt reprend ce module six ans plus tard**, en corrigeant ces défauts un par un, avec une spécification chiffrée et des mesures pour chaque exigence.

<!-- Photo du caisson ModSynth et de la carte FX de 2019 -->
<!-- ![ModSynth 2019](docs/2019/modsynth.jpg) -->

---

## Fonctionnalités visées

- **Accords au choix** : majeur, mineur, quinte, sus4, septièmes, et un mode « Harmoniques 2019 » pour comparer avec la version d'origine.
- **Renversements et étalement** des voix.
- **Écran tactile** : choix de l'accord, affichage de la note détectée et des notes générées, mélange et volume.
- **Contrôle Eurorack** :
  - CV accord, pour parcourir les accords avec un LFO ou un séquenceur ;
  - CV renversement ;
  - CV assignable (degré dans la gamme, mélange…) ;
  - entrée gate pour avancer dans une **progression d'accords** programmée (par exemple Am → F → C → G).
- **Mode gamme** : l'accord suit une tonalité (do majeur : do → do-mi-sol, ré → ré-fa-la), réglable à l'écran, en MIDI ou en CV.
- **Changements d'accord sans clic**, par fondu court.

Le détail est dans la **[spécification technique](docs/SPEC.md)** : exigences fonctionnelles, exigences de performance chiffrées et plan de validation.

---

## Matériel

| Élément | Choix |
|---|---|
| Carte | STM32F746G-DISCO (Cortex-M7 à 216 MHz, codec audio, écran tactile 4,3", SDRAM, flash Quad-SPI) |
| Interface Eurorack | Carte maison sur plaque à pastilles : 3 entrées CV −5/+5 V, 1 entrée gate, protections, atténuateur audio |
| Débogage | ST-LINK intégré, STM32CubeIDE |

L'interface Eurorack utilise un ampli-op rail à rail alimenté en 3,3 V : la sortie ne peut pas dépasser la plage de l'ADC, et les entrées supportent un branchement accidentel sur ±12 V. Schéma dans [`hardware/`](hardware/).

<!-- ![Carte d'interface](docs/photos/interface.jpg) -->

---

## Architecture logicielle

```
firmware/   drivers et intégration STM32 (C, HAL / BSP STM32CubeF7)
            codec audio (I2C), flux SAI en DMA, ADC des CV en DMA,
            gate en interruption, écran, tactile, flash Quad-SPI
dsp/        cœur DSP en C++ embarqué, indépendant du matériel
            PeriodDetector, CycleBuffer, Voice, ChordTable,
            ChordProgression, Harmonizer
tests/      tests unitaires du DSP, compilés et exécutés sur PC
```

Principes :

- **temps réel** : traitement par blocs de 1 ms en DMA, aucune allocation dynamique ni appel bloquant dans le traitement audio ;
- **priorités** : l'audio a la priorité la plus haute, l'interface graphique tourne dans la boucle principale ;
- **portabilité** : le cœur DSP ne dépend d'aucune bibliothèque matérielle et peut être porté sur une autre carte (par exemple Daisy, STM32H7) en réécrivant la seule couche d'accès au matériel ;
- **testabilité** : la détection de période, la génération des voix et la quantification des CV sont testées sur PC avec des signaux de synthèse.

---

## Feuille de route

- [ ] **V1 — Reproduction 2019** : la méthode d'origine, pour disposer d'un point de comparaison « avant ».
- [ ] **V2 — Accords au choix** : détection de période robuste, double tampon, lecture à vitesse fractionnaire interpolée, choix de l'accord à l'écran.
- [ ] **V3 — Contrôle Eurorack** : carte d'interface, entrées CV et gate, progressions d'accords sauvegardées en flash.
- [ ] **V4 — Mode gamme** : accords qui suivent une tonalité, réglage à l'écran, en MIDI ou en CV.
- [ ] **V5 — Qualité** : raccords synchronisés sur la période (type PSOLA), réduction des artefacts, optimisation de la charge processeur.

---

## Mesures

Chaque version est validée par des mesures, avec générateur de fonctions, oscilloscope et analyse spectrale :

| Exigence | Cible | Mesuré | Version |
|---|---|---|---|
| Latence entrée → sortie | ≤ 5 ms | — | — |
| Justesse des voix | ± 5 cents | — | — |
| Temps d'accrochage d'une nouvelle note | ≤ 30 ms à 80 Hz | — | — |
| Raies parasites (entrée sinus) | ≤ −40 dB | — | — |
| Délai gate → changement d'accord | ≤ 2 ms | — | — |
| Charge processeur | ≤ 60 % | — | — |

Les captures et le détail des protocoles sont dans [`docs/mesures/`](docs/mesures/).

---

## Démo

<!-- Lien vers la vidéo de démonstration (patch Eurorack : VCO → Harmonizer, séquenceur sur le gate, LFO sur la CV accord) -->
*À venir.*

---

## Compiler et flasher

*À compléter avec la V1.*

1. Ouvrir le dossier `firmware/` dans STM32CubeIDE.
2. Compiler en configuration `Release`.
3. Flasher par le ST-LINK intégré (câble USB sur le port ST-LINK de la carte).

Tests unitaires du DSP sur PC :

```bash
cd tests
cmake -B build && cmake --build build
./build/dsp_tests
```

---

## Auteur

**Clément Martin**

