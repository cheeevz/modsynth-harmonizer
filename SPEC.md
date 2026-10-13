# ModSynth Harmonizer — Spécification technique (version 2026)

Reprise du module FX/DSP du projet **ModSynth** (Polytech Sorbonne, EI2I, 2019-2020) : un module qui génère des accords en temps réel à partir d'un signal monophonique, avec des accords choisis à l'écran et modulables en tension de contrôle (CV) depuis un système modulaire Eurorack.

---

## 1. Contexte

En 2019-2020, une équipe de quatre élèves ingénieurs a conçu **ModSynth**, un synthétiseur modulaire au format Eurorack : séquenceur 16 pas MIDI, VCO, VCF, VCA, LFO, ADSR, alimentation ±15 V et un module d'effets numériques (FX/DSP). J'étais responsable du module DSP.

Le module FX tournait sur une carte STM32F4 Discovery, avec une carte d'accueil conçue sous Altium (étage d'entrée analogique vers l'ADC). Sa fonction la plus ambitieuse était la **génération de polyphonie** : à partir d'un signal monophonique, il restituait le signal accompagné de ses **harmoniques 3 et 5**, formant ainsi un accord.

**Méthode de 2019 :**
1. mesurer la période du signal à partir de ses maximums de tension et d'un timer ;
2. capturer un cycle dans un tableau et le recopier périodiquement ;
3. relire ces tableaux vers le DAC à des vitesses différentes pour obtenir les autres notes.

**Limites constatées :**
- détection de période sensible au bruit, donc hauteur instable ;
- clics à chaque répétition du cycle, quand la capture ne correspond pas exactement à une période ;
- seuls des rapports entiers (×3, ×5) étaient possibles, donc pas de choix du type d'accord ;
- les entrées CV 0/+5 V prévues au cahier des charges pour régler les paramètres n'ont pas été exploitées pour la génération d'accords ;
- pas d'exigences chiffrées dans le cahier des charges de 2019 : la validation se faisait par observation à l'oscilloscope.

Le rapport final concluait que le module devait être amélioré pour pouvoir **choisir le type d'accord** (majeur, mineur, septième…). Cette version 2026 répond à cette perspective, et rend ce choix pilotable par le reste du système modulaire.

---

## 2. Objectif

Concevoir un module qui :
- reçoit un signal audio monophonique et périodique (typiquement la sortie d'un VCO ou d'un synthétiseur) ;
- génère en temps réel jusqu'à **trois voix supplémentaires** formant un accord ;
- permet de **choisir l'accord** à l'écran tactile, et de le **moduler par des tensions de contrôle** venant d'un système Eurorack (séquenceur, LFO, enveloppe…) ;
- mélange ces voix avec le signal d'origine ;
- respecte des exigences **mesurables**, chacune associée à une méthode de test.

---

## 3. Plateforme cible

| Élément | Choix |
|---|---|
| Carte | STMicroelectronics STM32F746G-DISCO (32F746GDISCOVERY) |
| Microcontrôleur | STM32F746NG, Arm Cortex-M7 à 216 MHz, unité de calcul flottant, 1 Mo de flash, 340 Ko de RAM |
| Mémoire externe | SDRAM (64 Mbit accessibles) pour les tampons audio, flash Quad-SPI 128 Mbit pour les réglages et les progressions d'accords |
| Audio | Codec audio relié en SAI, jacks d'entrée et de sortie ligne |
| Interface | Écran couleur 4,3 pouces 480×272 avec dalle tactile capacitive, bouton utilisateur, LED utilisateur |
| Extensions | Connecteurs au format Arduino : entrées ADC (0 à 3,3 V) pour les CV, entrée numérique pour le gate, UART pour le MIDI |
| Carte fille | **Carte d'interface Eurorack** conçue pour le projet (section 4) |
| Langage | C pour les drivers (HAL et BSP STM32CubeF7), C++ embarqué pour le cœur DSP |
| Débogage | ST-LINK intégré à la carte, STM32CubeIDE |

**Portabilité :** le cœur DSP ne dépend d'aucune bibliothèque matérielle. Une couche d'abstraction (`AudioIO`, `Controls`, `CvInputs`) isole les drivers, ce qui permet de porter le projet sur un Daisy Seed (STM32H7) en réécrivant uniquement cette couche.

---

## 4. Carte d'interface Eurorack

Les tensions d'un système Eurorack (CV jusqu'à ±10 V, gates de 5 à 10 V, rails d'alimentation ±12 V) sont incompatibles avec les entrées du STM32 (0 à 3,3 V, sans tension négative). Une carte fille, enfichée sur le connecteur Arduino de la Discovery, assure l'adaptation et la protection.

**Réalisation :** montage sur **plaque à pastilles**, sans fabrication de circuit imprimé. Le schéma est saisi sous KiCad (gratuit) pour la documentation du dépôt. Le montage est relié au connecteur Arduino de la Discovery par des barrettes mâles soudées sur la plaque, ce qui la rend enfichable comme un shield.

**Choix de conception :** les ampli-ops sont alimentés en **3,3 V par la Discovery**, avec un modèle rail à rail. Leur sortie ne peut donc jamais dépasser la plage de l'ADC. Le montage inverseur garde ses entrées à une tension fixe d'environ 1,27 V : une tension négative en entrée ne pose donc pas de problème, même sans alimentation symétrique. Aucune alimentation ±12 V n'est nécessaire sur la plaque.

| Bloc | Fonction | Principe |
|---|---|---|
| Entrées CV ×3 | Adapter une tension −5 à +5 V à la plage 0 à 3,3 V de l'ADC | Ampli-op rail à rail en montage inverseur avec décalage : résistance d'entrée 100 kΩ, contre-réaction 30 kΩ (gain −0,3), entrée non inverseuse à environ 1,27 V par pont diviseur, pour centrer la sortie à 1,65 V (Vsortie = 1,3 × Vréf − 0,3 × Ventrée). Sortie entre 0,15 V et 3,15 V, condensateur en parallèle sur la contre-réaction pour filtrer le bruit. L'inversion est corrigée en logiciel. |
| Protection | Supporter une erreur de branchement sur les rails ±12 V | Résistance d'entrée de 100 kΩ qui limite le courant, ampli-op alimenté en 3,3 V qui borne la sortie, et diodes Schottky d'écrêtage (type BAT54S) vers 0 V et 3,3 V, avec une résistance série de 1 kΩ, en entrée d'ADC |
| Entrée gate ×1 | Détecter un front de gate ou de trigger Eurorack | Transistor NPN (type 2N3904 ou BC547) : résistance de base 100 kΩ, résistance de 100 kΩ entre base et masse, résistance de tirage de 10 kΩ vers 3,3 V sur le collecteur, diode 1N4148 entre masse et base pour protéger la jonction base-émetteur contre une tension négative. Seuil vers 1,3 V. La logique est inversée, ce qui est corrigé dans la configuration de l'interruption (front descendant). |
| Adaptation audio | Ramener le niveau Eurorack (environ 10 V crête à crête) au niveau ligne | Atténuateur passif (pont diviseur) en entrée. La sortie reste au niveau ligne ; un étage de gain vers le niveau Eurorack, qui demanderait une alimentation ±12 V, est une évolution possible. |
| Alimentation | Alimenter les ampli-ops et le détecteur de gate | 3,3 V et masse pris sur le connecteur Arduino de la Discovery, découplage par condensateurs de 100 nF au plus près des ampli-ops. La Discovery est alimentée en USB. |
| Connectique | Relier le montage au système Eurorack | Jacks 3,5 mm mono (type Thonkiconn) soudés sur la plaque ; masse commune avec le rack par les jacks. |

**Liste du matériel (ordre de grandeur : quelques euros) :**

| Composant | Quantité |
|---|---|
| Plaque à pastilles (format proche du shield Arduino) | 1 |
| Barrettes mâles au pas de 2,54 mm (pour enficher sur le connecteur Arduino) | 1 jeu |
| Ampli-op quadruple rail à rail, alimentation 3,3 V (type MCP6004) et support | 1 |
| Diodes Schottky doubles (type BAT54S) | 3 |
| Transistor NPN (type 2N3904 ou BC547) | 1 |
| Diode signal 1N4148 | 1 |
| Jacks 3,5 mm mono (type Thonkiconn) | 5 (3 CV, 1 gate, 1 audio) |
| Résistances à couche métallique (100 kΩ, 30 kΩ, 10 kΩ, 1 kΩ…) | une vingtaine |
| Condensateurs céramiques (100 nF, quelques nF) | une dizaine |

---

## 5. Exigences fonctionnelles

### Génération d'accords

| ID | Exigence |
|---|---|
| EF-01 | Le module reçoit un signal audio monophonique sur l'entrée ligne gauche. |
| EF-02 | Le module génère de 1 à 3 voix supplémentaires dont la hauteur dépend de la note d'entrée et de l'accord choisi. |
| EF-03 | Accords disponibles : majeur, mineur, quinte, quarte suspendue, septième de dominante, septième majeure, septième mineure, et un mode **« Harmoniques 2019 »** (harmoniques 3 et 5) pour comparer avec la version d'origine. |
| EF-04 | Chaque accord peut être **renversé** (fondamentale, premier, deuxième renversement) et **étalé** (voix décalées d'une octave) pour varier la position des voix. |
| EF-05 | La sortie stéréo reproduit le même signal sur les deux canaux (option : voix réparties gauche/droite). |
| EF-06 | Quand aucune période n'est détectée (silence, bruit), les voix générées sont coupées sans clic. |

### Interface tactile

| ID | Exigence |
|---|---|
| EF-07 | L'accord, le renversement et l'étalement se choisissent sur l'écran tactile ; l'écran affiche l'accord actif, la note d'entrée détectée, les notes générées et l'état des entrées CV. |
| EF-08 | Un curseur tactile règle le mélange entre signal d'origine et voix générées (de 100 % direct à 100 % voix). |
| EF-09 | Un curseur tactile règle le niveau de sortie global. |
| EF-10 | Pour chaque entrée CV, l'écran permet de choisir sa destination (accord, renversement, degré, mélange) et son atténuation. |

### Contrôle Eurorack

| ID | Exigence |
|---|---|
| EF-11 | **CV accord** : la tension parcourt la liste des accords. La plage est divisée en zones égales, une par accord. La valeur choisie à l'écran sert de point de départ et la CV la décale, comme un bouton et son entrée CV sur un module Eurorack. |
| EF-12 | **CV renversement** : la tension sélectionne le renversement et l'étalement de l'accord. |
| EF-13 | **CV assignable** : une troisième entrée peut être affectée au degré dans la gamme (mode gamme) ou au mélange direct/voix (variation continue). |
| EF-14 | **Entrée gate** : chaque front montant fait passer à l'accord suivant d'une **progression** de 1 à 16 accords, éditée à l'écran (par exemple Am → F → C → G). Un appui long sur le bouton utilisateur revient au premier accord. |
| EF-15 | Les progressions sont sauvegardées en flash Quad-SPI et restaurées au démarrage (8 emplacements). |
| EF-16 | Un changement d'accord commandé par CV ou gate est appliqué sans clic, par un fondu court des voix concernées. |

### Mode gamme et MIDI

| ID | Exigence |
|---|---|
| EF-17 | *(V4)* Mode **gamme** : l'accord suit une tonalité et un mode choisis (par exemple do majeur : do → do-mi-sol, ré → ré-fa-la). |
| EF-18 | *(V4)* La tonalité peut être choisie sur l'écran tactile, par une note MIDI reçue (interface MIDI à optocoupleur sur le connecteur Arduino), ou par la CV assignable. |

---

## 6. Exigences de performance

### Audio

| ID | Exigence | Valeur cible |
|---|---|---|
| EP-01 | Fréquence d'échantillonnage | 48 kHz |
| EP-02 | Résolution de conversion | 24 bits |
| EP-03 | Plage de fondamentales gérées | 80 Hz à 1 kHz (environ mi 2 à si 5) |
| EP-04 | Latence du signal d'origine, entrée vers sortie | ≤ 5 ms |
| EP-05 | Temps d'accrochage d'une nouvelle note | ≤ 2 périodes + 5 ms (soit ≤ 30 ms à 80 Hz) |
| EP-06 | Justesse des voix générées, en régime établi | ± 5 cents |
| EP-07 | Stabilité de hauteur sur une note tenue | variation ≤ 3 cents crête à crête |
| EP-08 | Raies parasites sur une entrée sinusoïdale | ≤ −40 dB par rapport à la voix la plus faible |
| EP-09 | Bruit de fond en sortie, entrée court-circuitée | ≤ −80 dBFS |
| EP-10 | Niveau d'entrée nominal | −10 dBV en entrée ligne ; 10 V crête à crête (niveau Eurorack) via la carte d'interface |
| EP-11 | Marge avant écrêtage à l'entrée | ≥ 12 dB au-dessus du niveau nominal |
| EP-12 | Absence de clic aux changements d'accord ou de note | aucune discontinuité visible à l'oscilloscope ni audible |
| EP-13 | Charge processeur du traitement audio | ≤ 60 % du temps disponible par bloc |

### Entrées CV et gate

| ID | Exigence | Valeur cible |
|---|---|---|
| EP-14 | Plage de tension CV exploitée | −5 V à +5 V |
| EP-15 | Tension supportée sans dommage sur les entrées CV et gate | −12 V à +12 V en continu |
| EP-16 | Impédance d'entrée CV et gate | ≥ 100 kΩ |
| EP-17 | Résolution de lecture CV après filtrage | ≤ 5 mV |
| EP-18 | Hystérésis entre deux zones d'accord | ≥ 50 mV (pas de basculement sur une tension stable placée en limite de zone) |
| EP-19 | Délai entre une variation de CV et le changement d'accord | ≤ 3 ms |
| EP-20 | Seuil de détection du gate | front montant détecté entre 1 V et 2 V ; gates de 5 V à 10 V acceptés |
| EP-21 | Délai entre un front de gate et le changement d'accord | ≤ 2 ms |
| EP-22 | Durée minimale de trigger détectée | ≤ 1 ms |
| EP-23 | Durée du fondu lors d'un changement d'accord | 2 à 10 ms, réglable |

---

## 7. Contraintes logicielles

- **Aucune allocation dynamique** dans le traitement audio : tous les tampons sont alloués statiquement au démarrage.
- **Pas d'exceptions ni de RTTI** (désactivés à la compilation).
- **Rien de bloquant dans la fonction de rappel audio** : pas d'affichage, pas d'accès carte SD, pas d'attente.
- Calculs en **`float` simple précision** (pas de `double` dans la boucle audio).
- Traitement par blocs de 48 échantillons (1 ms à 48 kHz), en DMA.
- **Double tampon** pour les cycles capturés : on lit l'un pendant qu'on remplit l'autre, et on échange à un instant de raccord propre.
- Lecture des voix par **accumulateur de phase à vitesse fractionnaire** (rapport 2^(n/12) pour n demi-tons), avec interpolation entre échantillons.
- **Entrées CV** lues par l'ADC en DMA, déclenché par un timer (au moins 2 kHz par voie), moyennées et filtrées, puis lues une fois par bloc audio. La quantification en zones d'accord applique l'hystérésis de l'EP-18.
- **Entrée gate** traitée par interruption sur front montant ; l'interruption se contente de lever un indicateur, appliqué au début du bloc audio suivant.
- Les changements d'accord, qu'ils viennent de l'écran, d'une CV ou du gate, passent par un même point d'entrée du `Harmonizer`, qui gère le fondu.
- Drivers écrits et documentés pour le projet : configuration du codec audio (I2C), flux audio SAI en DMA circulaire avec double tampon, ADC en DMA pour les CV, lecture de la dalle tactile (I2C), affichage (contrôleur LCD et accélérateur graphique), sauvegarde des réglages en flash Quad-SPI.
- Priorités d'interruption : audio en priorité la plus haute, puis gate et ADC, puis tactile. L'interface graphique est rafraîchie dans la boucle principale.
- Architecture séparée en modules testables : `AudioIO`, `CvInputs`, `GateInput`, `PeriodDetector`, `CycleBuffer`, `Voice`, `ChordTable`, `ChordProgression`, `Harmonizer`, `Controls`, `Ui`.
- Le cœur DSP et la logique de contrôle (`PeriodDetector`, `Voice`, `Harmonizer`, `ChordTable`, `ChordProgression`, quantification CV avec hystérésis) se compilent aussi sur PC pour des **tests unitaires**.

---

## 8. Feuille de route

| Version | Contenu | Exigences principales |
|---|---|---|
| **V1 — Reproduction 2019** | Même méthode qu'en 2019, mode harmoniques 3 et 5 uniquement. Sert de point de comparaison « avant ». | EF-01, EF-02, EF-03 (mode 2019) |
| **V2 — Accords au choix** | Détection de période robuste (autocorrélation ou YIN), double tampon, lecture fractionnaire interpolée, choix de l'accord, renversements et étalement sur l'écran tactile. | EF-02 à EF-09, EP-03 à EP-08, EP-12 |
| **V3 — Contrôle Eurorack** | Montage de la carte d'interface sur plaque à pastilles, entrées CV et gate, progressions d'accords, sauvegarde en flash. | EF-10 à EF-16, EP-14 à EP-23 |
| **V4 — Mode gamme** | Accords qui suivent une tonalité, choix de la tonalité sur l'écran, en MIDI ou par CV. | EF-17, EF-18 |
| **V5 — Qualité** | Raccords de cycle synchronisés sur la période (méthode type PSOLA), réduction des artefacts, optimisation de la charge processeur, façade au format Eurorack. | EP-08, EP-09, EP-13 |

---

## 9. Plan de validation

Matériel : générateur de fonctions (avec offset réglable), alimentation de laboratoire ou bus Eurorack, oscilloscope, interface audio USB et logiciel d'analyse spectrale (FFT) sur PC, source de synthèse (VCO ou synthétiseur), séquenceur ou LFO Eurorack.

### Audio

| Test | Exigences | Méthode |
|---|---|---|
| T-01 Latence | EP-04 | Impulsion ou front carré en entrée ; mesure de l'écart entrée/sortie sur les deux voies de l'oscilloscope. |
| T-02 Justesse | EP-06, EP-07 | Sinus à fréquence connue (80 Hz, 220 Hz, 440 Hz, 1 kHz) ; mesure des fréquences des voix par FFT ; calcul de l'écart en cents. |
| T-03 Accrochage | EP-05 | Saut de fréquence brutal en entrée ; mesure du temps jusqu'à stabilisation des voix à ± 5 cents. |
| T-04 Pureté | EP-08 | Sinus en entrée, accord majeur ; relevé des raies parasites par FFT. |
| T-05 Bruit de fond | EP-09 | Entrée court-circuitée, mélange à 100 % voix ; mesure du niveau de sortie. |
| T-06 Niveaux | EP-10, EP-11 | Sinus au niveau nominal, puis augmentation progressive jusqu'à l'écrêtage ; relevé de la marge, en entrée ligne et via la carte d'interface. |
| T-07 Clics | EP-12, EF-06, EF-16 | Changements d'accord et de note pendant la lecture, depuis l'écran, une CV et le gate ; observation de la sortie à l'oscilloscope en mode monocoup et écoute. |
| T-08 Charge CPU | EP-13 | Mesure du temps de traitement par bloc avec le compteur de cycles du Cortex-M7, ou par une broche basculée observée à l'oscilloscope. |

### CV et gate

| Test | Exigences | Méthode |
|---|---|---|
| T-09 Zones d'accord | EF-11, EP-14, EP-18 | Rampe lente de −5 V à +5 V puis retour ; relevé des tensions de changement d'accord à la montée et à la descente ; vérification de l'hystérésis. |
| T-10 Stabilité | EP-17, EP-18 | Tension continue placée en limite de zone pendant 60 s ; aucun changement d'accord ; relevé de la dispersion des valeurs lues. |
| T-11 Surtension | EP-15 | Application de −12 V puis +12 V sur chaque entrée ; vérification de la tension sur la broche du STM32 à l'oscilloscope et du bon fonctionnement après le test. |
| T-12 Impédance | EP-16 | Mesure du courant d'entrée sous ±5 V, ou de la chute de tension à travers une résistance série connue. |
| T-13 Réactivité CV | EP-19 | Échelon de tension en entrée CV et sortie audio sur l'oscilloscope ; mesure du délai jusqu'au début du fondu. |
| T-14 Gate | EF-14, EP-20, EP-21, EP-22 | Triggers de 1 ms et gates de 5 V et 10 V ; relevé du seuil, du délai jusqu'au changement d'accord et de l'enchaînement de la progression. |
| T-15 Sauvegarde | EF-15 | Édition d'une progression, coupure d'alimentation, redémarrage ; vérification de la restauration. |

### Système

| Test | Exigences | Méthode |
|---|---|---|
| T-16 Démonstration | EF-01 à EF-18 | Patch Eurorack complet (VCO vers le module, séquenceur sur le gate, LFO sur une CV) ; démonstration musicale enregistrée en vidéo. |
| T-17 Non-régression | EP de justesse, quantification CV, progressions | Tests unitaires sur PC avec signaux de synthèse (sinus, dent de scie, carré, bruit ajouté) et rampes de CV simulées. |

Chaque version publiée inclut un tableau de résultats (valeur mesurée, valeur cible, conforme ou non) et les captures correspondantes.

---

## 10. Traçabilité 2019 → 2026

| Cahier des charges 2019 | Version 2026 |
|---|---|
| Module DSP prenant le son en sortie du VCA par une entrée jack | EF-01, EP-10 (niveau ligne −10 dBV, et niveau Eurorack via la carte d'interface) |
| Paramètres réglables par potentiomètres et entrées CV 0/+5 V | EF-08 à EF-13 (curseurs tactiles et trois entrées CV −5/+5 V affectables) |
| Choix de l'effet par LED ou écran | EF-07, EF-10 (écran tactile) |
| Génération de polyphonie : harmoniques 3 et 5 | EF-03, mode « Harmoniques 2019 » conservé |
| Perspective : choisir le type d'accord | EF-02 à EF-04, EF-11, EF-14, EF-17 |
| Intégration dans le synthétiseur modulaire | Section 4 (carte d'interface Eurorack), EF-11 à EF-16 |
| Validation par observation à l'oscilloscope | Plan de validation chiffré (section 9), avec critères mesurables |
