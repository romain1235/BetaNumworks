# Guide de développement d'une application native (Epsilon / Upsilon / Beta)

> Ce guide couvre l'architecture complète du système d'applications, le framework UI **Escher** et tous les éléments disponibles pour construire une application native.(ce guide a été générer par une ia qui a analysé le code, il peux contenir des erreurs)

---

## Table des matières

1. [Architecture générale](#1-architecture-générale)
2. [Créer une application pas à pas](#2-créer-une-application-pas-à-pas)
3. [Système de navigation — les contrôleurs-conteneurs](#3-système-de-navigation--les-contrôleurs-conteneurs)
4. [Système d'événements — Responder](#4-système-dévénements--responder)
5. [Vues (Views) — base du rendu](#5-vues-views--base-du-rendu)
6. [Éléments d'interface Escher](#6-éléments-dinterface-escher)
   - [Vues texte](#61-vues-texte)
   - [Vues interactives (champs de saisie)](#62-vues-interactives-champs-de-saisie)
   - [Cellules de liste / tableau](#63-cellules-de-liste--tableau)
   - [Vues composites et containers](#64-vues-composites-et-containers)
   - [Widgets visuels](#65-widgets-visuels)
7. [Sources de données pour les tableaux](#7-sources-de-données-pour-les-tableaux)
8. [Internationalisation (i18n)](#8-internationalisation-i18n)
9. [Couleurs et métriques](#9-couleurs-et-métriques)
10. [Enregistrer l'app dans le build](#10-enregistrer-lapp-dans-le-build)
11. [Exemple complet minimal](#11-exemple-complet-minimal)

---

## 1. Architecture générale

```
ion_main()
  └─ AppsContainer          (Container global, gère tous les snapshots)
       ├─ Home::App
       ├─ MonApp::App        ← votre app
       │    ├─ App::Descriptor   (nom, icône)
       │    ├─ App::Snapshot     (état persistant, factory)
       │    └─ App              (racine : controllers + vues)
       └─ ...
```

### Cycle de vie d'une app

| Étape | Méthode | Description |
|-------|---------|-------------|
| Création | `Snapshot::unpack(Container*)` | Instancie l'`App` via placement new dans le buffer |
| Affichage | `viewWillAppear()` | Appelé avant que la vue soit visible |
| Focus | `didBecomeFirstResponder()` | L'app (ou un sous-contrôleur) reçoit les événements |
| Masquage | `viewDidDisappear()` | Vue masquée |
| Sauvegarde | `Snapshot::pack(App*)` | Sauvegarde l'état quand on quitte l'app |
| Reset | `Snapshot::reset()` | Réinitialise l'état persistant |

---

## 2. Créer une application pas à pas

### 2.1 Structure des fichiers

```
apps/monapp/
    app.h
    app.cpp
    main_controller.h
    main_controller.cpp
    monapp_icon.png
    Makefile
    base.en.i18n
    base.fr.i18n
    ...
```

### 2.2 `app.h`

```cpp
#ifndef MONAPP_APP_H
#define MONAPP_APP_H

#include "../shared/shared_app.h"
#include "main_controller.h"
#include <escher.h>

namespace MonApp {

class App : public ::App {
public:
  class Descriptor : public ::App::Descriptor {
  public:
    I18n::Message name() override;
    I18n::Message upperName() override;
    const Image * icon() override;
  };

  class Snapshot : public ::SharedApp::Snapshot {
  public:
    App * unpack(Container * container) override;
    Descriptor * descriptor() override;
  };

  static App * app() {
    return static_cast<App *>(Container::activeApp());
  }

private:
  App(Snapshot * snapshot);
  MainController m_controller;
  StackViewController m_stackViewController;
};

} // namespace MonApp
#endif
```

### 2.3 `app.cpp`

```cpp
#include "app.h"
#include "apps/apps_container.h"
#include "monapp_icon.h"        // généré depuis monapp_icon.png
#include "apps/i18n.h"
#include <escher/app.h>

namespace MonApp {

I18n::Message App::Descriptor::name() {
  return I18n::Message::MonAppName;
}

I18n::Message App::Descriptor::upperName() {
  return I18n::Message::MonAppNameUpper;
}

const Image * App::Descriptor::icon() {
  return ImageStore::MonAppIcon;
}

App * App::Snapshot::unpack(Container * container) {
  return new (container->currentAppBuffer()) App(this);
}

App::Descriptor * App::Snapshot::descriptor() {
  static Descriptor descriptor;
  return &descriptor;
}

App::App(Snapshot * snapshot) :
  ::App(snapshot, &m_stackViewController, I18n::Message::Warning),
  m_controller(&m_stackViewController, snapshot),
  m_stackViewController(&m_modalViewController, &m_controller)
{
}

} // namespace MonApp
```

> **Note :** `::App(snapshot, rootVC, warningMsg)` prend en 2e argument le **ViewController racine** (ici le StackViewController). `m_modalViewController` est hérité de `::App` — c'est lui qui gère les pop-ups modaux.

### 2.4 `main_controller.h`

```cpp
#ifndef MONAPP_MAIN_CONTROLLER_H
#define MONAPP_MAIN_CONTROLLER_H

#include <escher.h>

namespace MonApp {

class MainController : public ViewController,
                       public SimpleListViewDataSource,
                       public SelectableTableViewDataSource {
public:
  MainController(Responder * parent, SelectableTableViewDataSource * selectionDS);
  View * view() override { return &m_tableView; }
  void didBecomeFirstResponder() override;
  bool handleEvent(Ion::Events::Event event) override;

  // SimpleListViewDataSource
  int numberOfRows() const override;
  KDCoordinate cellHeight() override;
  HighlightCell * reusableCell(int index) override;
  int reusableCellCount() const override;
  void willDisplayCellForIndex(HighlightCell * cell, int index) override;

private:
  SelectableTableView m_tableView;
  static constexpr int k_cellCount = 5;
  MessageTableCellWithChevron<> m_cells[k_cellCount];
};

} // namespace MonApp
#endif
```

### 2.5 `main_controller.cpp`

```cpp
#include "main_controller.h"

namespace MonApp {

MainController::MainController(Responder * parent, SelectableTableViewDataSource * selDS)
  : ViewController(parent),
    m_tableView(this, this, selDS)
{}

void MainController::didBecomeFirstResponder() {
  if (m_tableView.selectedRow() < 0) {
    m_tableView.selectCellAtLocation(0, 0);
  }
  App::app()->setFirstResponder(&m_tableView);
}

int MainController::numberOfRows() const { return 3; }
KDCoordinate MainController::cellHeight() { return Metric::ParameterCellHeight; }
HighlightCell * MainController::reusableCell(int index) { return &m_cells[index]; }
int MainController::reusableCellCount() const { return k_cellCount; }

void MainController::willDisplayCellForIndex(HighlightCell * cell, int index) {
  MessageTableCellWithChevron<> * c = static_cast<MessageTableCellWithChevron<> *>(cell);
  I18n::Message labels[] = {
    I18n::Message::Option1,
    I18n::Message::Option2,
    I18n::Message::Option3
  };
  c->setMessage(labels[index]);
}

bool MainController::handleEvent(Ion::Events::Event event) {
  if (event == Ion::Events::OK || event == Ion::Events::EXE) {
    // Naviguer vers un sous-contrôleur
    // stackController()->push(&m_subController);
    return true;
  }
  return false;
}

} // namespace MonApp
```

### 2.6 `Makefile`

```makefile
apps += MonApp::App
app_headers += apps/monapp/app.h

apps_src += $(addprefix apps/monapp/,\
  app.cpp \
  main_controller.cpp \
)

i18n_files += $(call i18n_without_universal_for,monapp/base)

$(eval $(call depends_on_image,apps/monapp/app.cpp,apps/monapp/monapp_icon.png))
```

---

## 3. Système de navigation — les contrôleurs-conteneurs

### 3.1 `StackViewController`

Navigation en pile (push/pop). Affiche les en-têtes de navigation empilés en haut de l'écran.

```cpp
// Déclaration dans App
StackViewController m_stackViewController;

// Construction (parent, rootVC, couleurs optionnelles)
m_stackViewController(&m_modalViewController, &m_controller)

// Dans un contrôleur enfant, naviguer vers un autre contrôleur
StackViewController * stack = static_cast<StackViewController *>(parentResponder());
stack->push(&m_childController);

// Retour arrière (automatique sur Ion::Events::Back, ou manuel)
stack->pop();
```

Profondeur max : **4 niveaux** (`k_maxNumberOfStacks = 4`).

Les couleurs par défaut viennent de `Palette::SubMenuText / SubMenuBackground / SubMenuBorder`.

### 3.2 `TabViewController`

Navigation par onglets (barre d'onglets en bas de l'écran). Supporte 3 ou 4 onglets.

```cpp
TabViewController m_tabVC;
// Construction : (parent, dataSource, vc1, vc2, vc3, vc4_optional)
m_tabVC(&m_modalViewController, &m_tabDS, &m_tab1, &m_tab2, &m_tab3)

m_tabVC.setActiveTab(0); // changer d'onglet
```

La source de données (`TabViewDataSource`) stocke l'onglet actif.

### 3.3 `ModalViewController`

Intégré dans `::App` en tant que `m_modalViewController`. Permet d'afficher un contrôleur par-dessus l'UI principale.

```cpp
// Afficher un modal (alignement vertical, horizontal, marges)
app->displayModalViewController(&m_popUp, 0.5f, 0.5f,
  Metric::PopUpTopMargin, Metric::PopUpLeftMargin,
  Metric::ModalBottomMargin, Metric::PopUpRightMargin);

// Fermer
app->dismissModalViewController();
```

### 3.4 `ButtonRowController`

Wrapping d'un `ViewController` principal avec une rangée de boutons en haut ou en bas.

```cpp
ButtonRowController m_buttonRowVC;
// (parent, mainVC, delegate, Position::Bottom, Style::EmbossedGray, Size::Large)
m_buttonRowVC(&m_stackVC, &m_mainVC, this,
              ButtonRowController::Position::Bottom,
              ButtonRowController::Style::EmbossedGray,
              ButtonRowController::Size::Large)
```

Le délégué doit implémenter `ButtonRowDelegate` : fournir le nombre de boutons, leur message et leur invocation.

### 3.5 `AlternateEmptyViewController`

Affiche un message d'état vide quand le contenu principal est vide.

```cpp
AlternateEmptyViewController m_alternateVC;
// (parent, mainVC, delegate)
m_alternateVC(&m_stackVC, &m_mainVC, &m_mainVC /* delegate */)
```

Le délégué implémente `AlternateEmptyViewDelegate` :

```cpp
bool isEmpty() const override { return m_count == 0; }
I18n::Message emptyMessage() override { return I18n::Message::NoItems; }
Responder * defaultController() override { return this; }
```

### 3.6 `NestedMenuController`

Menu hiérarchique (arborescence). Hérité de `StackViewController`. Utilisé pour les toolboxes.

```cpp
// Implémenter :
bool selectLeaf(int selectedRow, bool quitToolbox) override;
// + fournir les données via MessageTree / ToolboxMessageTree
```

---

## 4. Système d'événements — Responder

Toute classe qui reçoit des événements hérite de `Responder`.

```cpp
class MonControler : public ViewController { // ViewController étend Responder
public:
  bool handleEvent(Ion::Events::Event event) override {
    if (event == Ion::Events::OK) { /* … */ return true; }
    if (event == Ion::Events::Back) { /* … */ return true; }
    return false; // passe l'événement au parent
  }

  void didBecomeFirstResponder() override {
    // Appelé quand ce contrôleur devient actif
    App::app()->setFirstResponder(&m_tableView); // déléguer à la vue
  }

  void willResignFirstResponder() override { /* sauvegarde de l'état */ }
};
```

**Chaîne de Responders :** l'événement remonte du `firstResponder` vers ses parents via `m_parentResponder` jusqu'à être consommé (`return true`).

**Événements courants :**

| Événement | Description |
|-----------|-------------|
| `Ion::Events::OK` | Touche OK |
| `Ion::Events::EXE` | Touche EXE |
| `Ion::Events::Back` | Retour arrière |
| `Ion::Events::Up/Down/Left/Right` | Navigation |
| `Ion::Events::Plus/Minus` | +/− |
| `Ion::Events::Home` | Retour au menu principal |
| `Ion::Events::Backspace` | Effacement |

---

## 5. Vues (Views) — base du rendu

Toutes les vues héritent de `View`. Une vue **ne gère pas les événements** (c'est le rôle du `Responder`).

```cpp
class MaVue : public View {
public:
  // Dessin custom
  void drawRect(KDContext * ctx, KDRect rect) const override {
    ctx->fillRect(bounds(), KDColorWhite);
    ctx->drawString("Hello", KDPoint(10, 10), KDFont::LargeFont,
                    KDColorBlack, KDColorWhite);
  }

  // Déclarer les sous-vues
  int numberOfSubviews() const override { return 1; }
  View * subviewAtIndex(int index) override { return &m_label; }

  // Positionner les sous-vues
  void layoutSubviews(bool force = false) override {
    m_label.setFrame(KDRect(0, 0, bounds().width(), 20), force);
  }

private:
  MessageTextView m_label;
};
```

**Règles importantes :**
- Une vue ne peut dessiner qu'à l'intérieur de son propre `bounds()`.
- `setFrame(KDRect, bool force)` positionne une sous-vue — toujours appelé depuis `layoutSubviews`.
- `markRectAsDirty(rect)` force le redessin d'une zone.

**Primitives de dessin (`KDContext`) :**

```cpp
ctx->fillRect(KDRect(x, y, w, h), color);
ctx->drawString(text, KDPoint(x, y), font, textColor, bgColor);
ctx->blendRectWithMask(rect, color, mask, rect);
```

---

## 6. Éléments d'interface Escher

### 6.1 Vues texte

#### `TextView` (abstraite)
Base commune pour toutes les vues texte. Gère la police, l'alignement, les couleurs.

```cpp
// Paramètres communs :
// font         : KDFont::SmallFont | KDFont::LargeFont
// horizontalAlignment : 0.0 (gauche) … 0.5 (centre) … 1.0 (droite)
// verticalAlignment   : idem
// textColor    : ex. Palette::PrimaryText
// backgroundColor : ex. Palette::ListCellBackground
```

#### `MessageTextView`
Texte i18n statique. **La plus utilisée.**

```cpp
MessageTextView m_label;
// Construction :
MessageTextView(KDFont::SmallFont, I18n::Message::MyMessage,
                0.5f, 0.5f, KDColorBlack, KDColorWhite);

m_label.setMessage(I18n::Message::MyMessage);
m_label.setText("texte brut");       // override le message i18n
m_label.setTextColor(KDColorRed);
m_label.setBackgroundColor(KDColorWhite);
m_label.setAlignment(0.5f, 0.5f);
m_label.setFont(KDFont::LargeFont);
```

#### `BufferTextView`
Texte dynamique dans un buffer interne (max 256 chars).

```cpp
BufferTextView m_text;
// Construction :
BufferTextView(KDFont::SmallFont, 0.0f, 0.5f, KDColorBlack, KDColorWhite);

m_text.setText("valeur : 42");
m_text.appendText(" unité");
// m_text.text() → pointeur vers le buffer
```

#### `PointerTextView`
Texte pointant vers un `const char *` externe (pas de copie).

```cpp
PointerTextView m_ptr;
m_ptr.setText(monPointeurExterne);  // stocke le pointeur, ne copie pas
```

#### `ExpressionView`
Affiche un layout mathématique `Poincare::Layout` (expression rendue graphiquement).

```cpp
ExpressionView m_exprView;
// Construction :
ExpressionView(0.0f, 0.5f, Palette::PrimaryText, Palette::ListCellBackground);

Poincare::Layout layout = /* ... */;
m_exprView.setLayout(layout);
m_exprView.setTextColor(KDColorBlack);
m_exprView.setBackgroundColor(KDColorWhite);
m_exprView.setAlignment(0.5f, 0.5f);
```

#### `ImageView`
Affiche une image PNG compilée.

```cpp
ImageView m_img;
m_img.setImage(ImageStore::MonIcon);

// Ou depuis des données brutes :
m_img.setImage(rawData, dataLength);
```

#### `SolidColorView`
Rectangle d'une couleur unie.

```cpp
SolidColorView m_bg(KDColorBlue);
m_bg.setColor(Palette::Toolbar);
m_bg.reload();  // force le redessin
```

---

### 6.2 Vues interactives (champs de saisie)

Ces vues héritent également de `Responder` et gèrent la saisie clavier.

#### `TextField`
Champ de saisie texte mono-ligne.

```cpp
// Buffers nécessaires :
char m_textBuffer[TextField::maxBufferSize()];       // texte affiché
char m_draftBuffer[TextField::maxBufferSize()];      // brouillon d'édition

TextField m_field;
// Construction :
TextField(parentResponder,
          m_textBuffer, sizeof(m_textBuffer),
          sizeof(m_draftBuffer),
          inputEventHandlerDelegate, // nullptr si pas de toolbox
          textFieldDelegate,         // votre contrôleur implémentant TextFieldDelegate
          KDFont::LargeFont,
          0.0f, 0.5f,               // alignement
          Palette::PrimaryText,
          Palette::BackgroundHard);

m_field.setText("42");
m_field.setEditing(true);
// Délégué : TextFieldDelegate
//   textFieldShouldFinishEditing(TextField*, Ion::Events::Event)
//   textFieldDidFinishEditing(TextField*, const char*, Ion::Events::Event)
//   textFieldDidAbortEditing(TextField*)
```

#### `TextArea`
Champ de saisie texte multi-lignes (utilisé dans l'éditeur Python).

```cpp
TextArea m_area(parentResponder, &m_contentView, KDFont::SmallFont);
m_area.setDelegates(inputEventHandlerDelegate, textAreaDelegate);
m_area.setText(buffer, bufferSize);
```

#### `LayoutField`
Champ de saisie pour expressions mathématiques (rendu Poincare).

```cpp
LayoutField m_layoutField;
// Construction :
LayoutField(parentResponder, inputEventHandlerDelegate, layoutFieldDelegate);

m_layoutField.setEditing(true);
m_layoutField.setLayout(monLayout);
m_layoutField.clearLayout();
Poincare::Layout layout = m_layoutField.layout();
```

#### `ExpressionField`
Combine `TextField` et `LayoutField` : bascule automatiquement entre les deux modes.

```cpp
ExpressionField m_exprField;
// Construction :
ExpressionField(parentResponder, inputHandler,
                textFieldDelegate, layoutFieldDelegate);

m_exprField.setEditing(true, true /* reinitDraft */);
const char * text = m_exprField.text();
```

---

### 6.3 Cellules de liste / tableau

Toutes les cellules héritent de `HighlightCell` (et souvent de `TableCell`). Elles sont recyclées par `SelectableTableView`.

#### Hiérarchie des cellules

```
HighlightCell
└─ TableCell (label + accessory + subAccessory, avec bordures)
   ├─ MessageTableCell<T>
   │  ├─ MessageTableCellWithChevron<T>      ← label + flèche droite
   │  ├─ MessageTableCellWithMessage<T>      ← label + sous-texte
   │  ├─ MessageTableCellWithExpression      ← label + expression math
   │  ├─ MessageTableCellWithBuffer          ← label + texte dynamique
   │  ├─ MessageTableCellWithSwitch          ← label + toggle switch
   │  ├─ MessageTableCellWithGauge           ← label + jauge
   │  ├─ MessageTableCellWithEditableText    ← label + TextField intégré
   │  ├─ MessageTableCellWithChevronAndBuffer
   │  ├─ MessageTableCellWithChevronAndMessage
   │  └─ MessageTableCellWithChevronAndExpression
   ├─ EvenOddCell (fond alternant pair/impair)
   │  ├─ EvenOddBufferTextCell
   │  ├─ EvenOddMessageTextCell
   │  ├─ EvenOddExpressionCell
   │  ├─ EvenOddEditableTextCell
   │  └─ EvenOddCellWithEllipsis
   └─ ExpressionTableCell
      ├─ ExpressionTableCellWithExpression
      └─ ExpressionTableCellWithPointer
```

#### `MessageTableCell<T>` — cellule label seul

```cpp
MessageTableCell<> m_cell;
// ou avec police personnalisée :
MessageTableCell<MessageTextView> m_cell(I18n::Message::MyLabel, KDFont::SmallFont);

m_cell.setMessage(I18n::Message::MyLabel);
m_cell.setTextColor(KDColorGray);
m_cell.setBackgroundColor(KDColorWhite);
m_cell.setMessageFont(KDFont::LargeFont);
```

#### `MessageTableCellWithChevron<>` — cellule avec flèche (navigation)

```cpp
MessageTableCellWithChevron<> m_cell(I18n::Message::MyLabel, KDFont::SmallFont);
m_cell.setMessage(I18n::Message::MyLabel);
// La flèche (ChevronView) est automatique en accessoryView
```

#### `MessageTableCellWithMessage<>` — cellule label + sous-texte

```cpp
MessageTableCellWithMessage<> m_cell(I18n::Message::MyLabel,
                                      TableCell::Layout::Vertical);
m_cell.setMessage(I18n::Message::MyLabel);
m_cell.setAccessoryMessage(I18n::Message::MyValue);
m_cell.setAccessoryTextColor(KDColorGray);
```

#### `MessageTableCellWithBuffer` — cellule label + texte dynamique

```cpp
MessageTableCellWithBuffer m_cell;
m_cell.setMessage(I18n::Message::MyLabel);
m_cell.setAccessoryText("42.0");
```

#### `MessageTableCellWithSwitch` — cellule avec toggle

```cpp
MessageTableCellWithSwitch m_cell(I18n::Message::EnableOption);
// Dans willDisplayCellForIndex :
SwitchView * sw = static_cast<SwitchView*>(m_cell.accessoryView());
sw->setState(monParametre);
```

#### `MessageTableCellWithGauge` — cellule avec jauge de progression

```cpp
MessageTableCellWithGauge m_cell;
m_cell.setMessage(I18n::Message::Progress);
GaugeView * gauge = static_cast<GaugeView*>(m_cell.accessoryView());
gauge->setLevel(0.75f); // 0.0 … 1.0
```

#### `MessageTableCellWithExpression` — cellule label + expression math

```cpp
MessageTableCellWithExpression m_cell(I18n::Message::Result, KDFont::LargeFont);
m_cell.setMessage(I18n::Message::Result);
m_cell.setLayout(monLayout);
```

#### `EvenOddBufferTextCell` — texte pair/impair (tableaux de données)

```cpp
EvenOddBufferTextCell m_cell;
m_cell.setEven(index % 2 == 0);
m_cell.setText("3.14159");
```

#### `Button` — bouton cliquable

```cpp
Button m_button;
// Construction :
Button(parentResponder,
       I18n::Message::OK,
       Invocation([](void * ctx, void * sender) -> bool {
         // action
         return true;
       }, this),
       KDFont::SmallFont,
       Palette::ButtonText);

m_button.setMessage(I18n::Message::Cancel);
```

---

### 6.4 Vues composites et containers

#### `SelectableTableView`
Vue tableau avec sélection et défilement. C'est la vue principale pour toute liste ou grille.

```cpp
SelectableTableView m_tableView;
// Construction :
// template : SelectableTableView(T * p) où T implémente
//   TableViewDataSource + SelectableTableViewDataSource + SelectableTableViewDelegate

SelectableTableView m_tableView(this); // si le contrôleur implémente les 3

// Construction explicite :
SelectableTableView(parentResponder, dataSource, selectionDataSource, delegate);

// Utilisation :
m_tableView.selectCellAtLocation(colonne, ligne);
m_tableView.selectRow(0);
m_tableView.selectColumn(0);
m_tableView.reloadData();
m_tableView.deselectTable();
HighlightCell * cell = m_tableView.selectedCell();
int row = m_tableView.selectedRow();
int col = m_tableView.selectedColumn();
```

#### `ScrollView`
Vue avec défilement (contenu plus grand que l'écran).

```cpp
ScrollView m_scroll;
// Construction : (contentView, dataSource)

m_scroll.setTopMargin(10);
m_scroll.setBottomMargin(10);
m_scroll.setLeftMargin(20);
m_scroll.setRightMargin(20);
m_scroll.setMargins(10); // tous égaux
```

Décorateurs disponibles : `ScrollView::BarDecorator` (barres de défilement), `ScrollView::ArrowDecorator` (flèches).

#### `PopUpController`
Dialogue de confirmation avec boutons OK / Annuler.

```cpp
PopUpController m_popUp;
// Construction : (numberOfLines, okInvocation)
PopUpController(2, Invocation(monAction, this));

// Afficher :
m_popUp.setMessage(0, I18n::Message::AreYouSure);
m_popUp.setMessage(1, I18n::Message::ActionDescription);
app->displayModalViewController(&m_popUp, 0.5f, 0.5f,
  Metric::PopUpTopMargin, Metric::PopUpLeftMargin,
  Metric::ModalBottomMargin, Metric::PopUpRightMargin);
```

---

### 6.5 Widgets visuels

#### `SwitchView` — toggle on/off

```cpp
SwitchView m_switch;
m_switch.setState(true);  // ToggleableView::setState
bool on = m_switch.state();
// Taille fixe : 22×12 px (k_switchWidth × k_switchHeight)
```

#### `ChevronView` — flèche de navigation (›)

```cpp
ChevronView m_chevron;
// Pas de paramètre, se dessine automatiquement
// Taille minimale calculée par minimalSizeForOptimalDisplay()
```

#### `GaugeView` — barre de progression

```cpp
GaugeView m_gauge;
m_gauge.setLevel(0.5f);           // 0.0 → 1.0
m_gauge.setBackgroundColor(Palette::Toolbar);
// Hauteur : k_indicatorDiameter = 10 px
```

#### `EllipsisView` — indicateur "…" (options supplémentaires)

```cpp
EllipsisView m_ellipsis;
// Se dessine automatiquement, largeur fixe : Metric::EllipsisCellWidth = 37
```

#### `IconView` — icône d'application

```cpp
IconView m_icon;
m_icon.setImage(ImageStore::MonIcon);
```

#### `KeyView` — représentation d'une touche physique

```cpp
KeyView m_key;
m_key.setKey(Ion::Keyboard::Key::A);
```

#### `ToggleableDotView` — point toggleable (radio-button)

```cpp
ToggleableDotView m_dot;
m_dot.setState(true);
```

---

## 7. Sources de données pour les tableaux

### 7.1 Hiérarchie

```
TableViewDataSource                   ← interface complète (grille 2D)
├─ ListViewDataSource                 ← liste 1 colonne
│  └─ SimpleListViewDataSource        ← liste 1 colonne, hauteur fixe
└─ SimpleTableViewDataSource          ← grille, cellules de même taille
```

### 7.2 `SimpleListViewDataSource` — le plus simple pour une liste

Implémenter dans votre contrôleur :

```cpp
class MonCtrl : public ViewController,
                public SimpleListViewDataSource,
                public SelectableTableViewDataSource {
public:
  // Obligatoires :
  int numberOfRows() const override { return 5; }
  KDCoordinate cellHeight() override { return Metric::ParameterCellHeight; } // 35px
  HighlightCell * reusableCell(int index) override { return &m_cells[index]; }
  int reusableCellCount() const override { return k_nbCells; }
  void willDisplayCellForIndex(HighlightCell * cell, int index) override {
    static_cast<MessageTableCell<> *>(cell)->setMessage(maListe[index]);
  }

private:
  static constexpr int k_nbCells = 6; // nombre de cellules visibles à l'écran
  MessageTableCell<> m_cells[k_nbCells];
};
```

> **Règle cellules :** `reusableCellCount` doit être ≥ au nombre de cellules **visibles** simultanément (pas le nombre total de lignes). En pratique : `ceil(écranHeight / cellHeight) + 1`.

### 7.3 `ListViewDataSource` — liste avec hauteurs variables

```cpp
class MonCtrl : public ViewController,
                public ListViewDataSource,
                public SelectableTableViewDataSource {
public:
  int numberOfRows() const override;
  KDCoordinate rowHeight(int j) override;   // hauteur par ligne
  HighlightCell * reusableCell(int index, int type) override;
  int reusableCellCount(int type) override;
  int typeAtLocation(int i, int j) override; // type de cellule par position
  void willDisplayCellForIndex(HighlightCell * cell, int index) override;
};
```

### 7.4 `SimpleTableViewDataSource` — grille 2D uniforme

```cpp
class MonCtrl : public ViewController,
                public SimpleTableViewDataSource,
                public SelectableTableViewDataSource {
public:
  int numberOfRows() const override;
  int numberOfColumns() const override;
  KDCoordinate cellHeight() override;
  KDCoordinate cellWidth() override;
  HighlightCell * reusableCell(int index) override;
  int reusableCellCount() const override;
  void willDisplayCellAtLocation(HighlightCell * cell, int col, int row) override;
};
```

### 7.5 `SelectableTableViewDataSource`

Stocke la sélection courante. Peut être héritée par le `Snapshot` pour la persistance :

```cpp
class App::Snapshot : public ::SharedApp::Snapshot,
                      public SelectableTableViewDataSource {
  // selectedRow() / selectedColumn() sont automatiquement persistés
};
```

### 7.6 `SelectableTableViewDelegate`

Recevoir les notifications de changement de sélection :

```cpp
void tableViewDidChangeSelection(SelectableTableView * t,
                                  int prevX, int prevY,
                                  bool withinTemporarySelection) override {
  if (!withinTemporarySelection) {
    // mise à jour de l'UI selon la sélection
  }
}
```

---

## 8. Internationalisation (i18n)

### 8.1 Fichiers `.i18n`

Créer `apps/monapp/base.en.i18n` :

```
MonAppName = "My App"
MonAppNameUpper = "MY APP"
Option1 = "Option 1"
Option2 = "Option 2"
```

Et `apps/monapp/base.fr.i18n` :

```
MonAppName = "Mon App"
MonAppNameUpper = "MON APP"
Option1 = "Option 1"
Option2 = "Option 2"
```

Pour les messages partagés entre tous les locales, utiliser `base.universal.i18n`.

### 8.2 Utilisation dans le code

```cpp
#include <apps/i18n.h>

I18n::Message::MonAppName   // accès au message
I18n::translate(I18n::Message::MonAppName)  // → const char *
```

### 8.3 Déclaration dans le Makefile

```makefile
# Tous les locales sauf universal :
i18n_files += $(call i18n_without_universal_for,monapp/base)

# Tous les locales + universal :
i18n_files += $(call i18n_with_universal_for,monapp/atomsName)
```

---

## 9. Couleurs et métriques

### 9.1 Palette de couleurs (`Palette::`)

```
Palette::PrimaryText           Texte principal
Palette::SecondaryText         Texte secondaire (grisé)
Palette::BackgroundHard        Fond de l'écran (blanc)
Palette::ListCellBackground    Fond d'une cellule de liste
Palette::Toolbar               Barre de titre
Palette::SubMenuBackground     Fond du menu en stack
Palette::SubMenuText           Texte du menu en stack
Palette::SubMenuBorder         Séparateur du menu en stack
Palette::ButtonText            Texte d'un bouton
Palette::ButtonBackgroundSelected       Fond bouton sélectionné
Palette::ButtonBackgroundSelectedHighContrast  Haute visibilité
Palette::Select                Fond de sélection
Palette::AtomColor[type]       Couleur par type d'atome (tableau périodique)
```

Couleurs Kandinsky constantes :
```cpp
KDColorWhite, KDColorBlack, KDColorRed, KDColorGreen,
KDColorBlue, KDColorYellow, KDColor::RGB24(0xRRGGBB)
```

### 9.2 Métriques (`Metric::`)

| Constante | Valeur | Usage |
|-----------|--------|-------|
| `TitleBarHeight` | 18 | Hauteur de la barre de titre |
| `TabHeight` | 27 | Hauteur de la barre d'onglets |
| `StackTitleHeight` | 20 | Hauteur d'un en-tête de stack |
| `ParameterCellHeight` | 35 | Hauteur standard d'une cellule |
| `ToolboxRowHeight` | 40 | Hauteur d'une ligne de toolbox |
| `StoreRowHeight` | 50 | Hauteur d'une ligne de store |
| `TableCellVerticalMargin` | 3 | Marge verticale dans TableCell |
| `TableCellHorizontalMargin` | 10 | Marge horizontale dans TableCell |
| `CommonLeftMargin` | 20 | Marge gauche standard |
| `CommonRightMargin` | 20 | Marge droite standard |
| `CommonTopMargin` | 15 | Marge haute standard |
| `CommonBottomMargin` | 15 | Marge basse standard |
| `EllipsisCellWidth` | 37 | Largeur de la cellule "…" |

### 9.3 Polices

```cpp
KDFont::SmallFont   // petite police (listes, méta-données)
KDFont::LargeFont   // grande police (valeurs, titres)
```

---

## 10. Enregistrer l'app dans le build

### 10.1 Ajouter l'app à `EPSILON_APPS`

Dans `build/config.mak` ou via la ligne de commande :

```makefile
EPSILON_APPS = calculation graph ... monapp
```

### 10.2 Déclarer le snapshot dans `AppsContainerStorage`

Les macros `APPS_CONTAINER_SNAPSHOT_DECLARATIONS`, `APPS_CONTAINER_SNAPSHOT_CONSTRUCTORS`, `APPS_CONTAINER_SNAPSHOT_LIST` et `APPS_CONTAINER_SNAPSHOT_COUNT` sont générées automatiquement par le build system à partir de la variable `apps` définie dans les Makefiles des apps.

Dans `apps/monapp/Makefile` :

```makefile
apps += MonApp::App
app_headers += apps/monapp/app.h
```

Ces deux lignes suffisent — le build système génère le reste automatiquement.

### 10.3 Icône

L'image `monapp_icon.png` doit être une image **55×56 pixels**, format PNG. Elle sera compilée en `monapp_icon.h` / `ImageStore::MonAppIcon` via le script de conversion d'images.

---

## 11. Exemple complet minimal

Une app avec une liste de 3 entrées navigables, chacune ouvrant un écran de détail.

```
apps/exemple/
    app.h / app.cpp
    list_controller.h / list_controller.cpp
    detail_controller.h / detail_controller.cpp
    exemple_icon.png
    Makefile
    base.en.i18n / base.fr.i18n
```

### `list_controller.h`

```cpp
#pragma once
#include <escher.h>

namespace Exemple {

class DetailController;

class ListController : public ViewController,
                       public SimpleListViewDataSource,
                       public SelectableTableViewDataSource {
public:
  ListController(Responder * parent);
  View * view() override { return &m_tableView; }
  void didBecomeFirstResponder() override;
  bool handleEvent(Ion::Events::Event event) override;

  int numberOfRows() const override { return 3; }
  KDCoordinate cellHeight() override { return Metric::ParameterCellHeight; }
  HighlightCell * reusableCell(int index) override { return &m_cells[index]; }
  int reusableCellCount() const override { return k_nbCells; }
  void willDisplayCellForIndex(HighlightCell * cell, int index) override;

private:
  static constexpr int k_nbCells = 3;
  SelectableTableView m_tableView;
  MessageTableCellWithChevron<> m_cells[k_nbCells];
  DetailController m_detailController;
};

} // namespace Exemple
```

### `list_controller.cpp`

```cpp
#include "list_controller.h"
#include "detail_controller.h"

namespace Exemple {

static I18n::Message k_labels[] = {
  I18n::Message::Item1,
  I18n::Message::Item2,
  I18n::Message::Item3,
};

ListController::ListController(Responder * parent)
  : ViewController(parent),
    m_tableView(this, this, this),
    m_detailController(this)
{}

void ListController::didBecomeFirstResponder() {
  if (m_tableView.selectedRow() < 0) {
    m_tableView.selectCellAtLocation(0, 0);
  }
  App::app()->setFirstResponder(&m_tableView);
}

void ListController::willDisplayCellForIndex(HighlightCell * cell, int index) {
  static_cast<MessageTableCellWithChevron<> *>(cell)->setMessage(k_labels[index]);
}

bool ListController::handleEvent(Ion::Events::Event event) {
  if (event == Ion::Events::OK || event == Ion::Events::EXE) {
    m_detailController.setIndex(m_tableView.selectedRow());
    StackViewController * stack = static_cast<StackViewController *>(parentResponder());
    stack->push(&m_detailController);
    return true;
  }
  return false;
}

} // namespace Exemple
```

### `detail_controller.h`

```cpp
#pragma once
#include <escher.h>

namespace Exemple {

class DetailController : public ViewController {
public:
  DetailController(Responder * parent);
  void setIndex(int index) { m_index = index; }
  View * view() override { return &m_view; }
  void viewWillAppear() override;
  const char * title() override;

private:
  class ContentView : public View {
  public:
    void setIndex(int index);
    void drawRect(KDContext * ctx, KDRect rect) const override;
  private:
    int m_index = 0;
  };

  ContentView m_view;
  int m_index = 0;
};

} // namespace Exemple
```

### `Makefile`

```makefile
apps += Exemple::App
app_headers += apps/exemple/app.h

apps_src += $(addprefix apps/exemple/,\
  app.cpp \
  list_controller.cpp \
  detail_controller.cpp \
)

i18n_files += $(call i18n_without_universal_for,exemple/base)

$(eval $(call depends_on_image,apps/exemple/app.cpp,apps/exemple/exemple_icon.png))
```

---

## Récapitulatif des classes clés

| Classe | Rôle |
|--------|------|
| `App` | Racine de l'application |
| `App::Descriptor` | Métadonnées (nom, icône) |
| `App::Snapshot` | État persistant + factory |
| `ViewController` | Contrôleur de vue (gère une vue + événements) |
| `StackViewController` | Navigation en pile |
| `TabViewController` | Navigation par onglets |
| `ModalViewController` | Superposition modale |
| `ButtonRowController` | Rangée de boutons |
| `AlternateEmptyViewController` | Affichage "liste vide" |
| `Responder` | Récepteur d'événements |
| `View` | Élément graphique de base |
| `SelectableTableView` | Liste/grille interactive |
| `ScrollView` | Vue scrollable |
| `MessageTextView` | Texte i18n statique |
| `BufferTextView` | Texte dynamique |
| `ExpressionView` | Expression mathématique |
| `ImageView` | Image PNG |
| `SolidColorView` | Rectangle coloré |
| `TextField` | Saisie texte mono-ligne |
| `TextArea` | Saisie texte multi-lignes |
| `LayoutField` | Saisie expression math |
| `Button` | Bouton cliquable |
| `MessageTableCell<>` | Cellule texte |
| `MessageTableCellWithChevron<>` | Cellule navigation |
| `MessageTableCellWithSwitch` | Cellule toggle |
| `MessageTableCellWithMessage<>` | Cellule label + sous-texte |
| `MessageTableCellWithBuffer` | Cellule label + valeur dynamique |
| `MessageTableCellWithGauge` | Cellule avec jauge |
| `PopUpController` | Dialogue OK/Annuler |
| `GaugeView` | Barre de progression |
| `SwitchView` | Toggle visuel |
| `ChevronView` | Flèche de navigation |
