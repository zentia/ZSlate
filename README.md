# ZSlate

A lightweight, retained-mode UI framework for C++20 applications.

## Features

- **Retained-mode widget tree** - Declarative UI construction with automatic layout
- **Rich widget library** - 20+ built-in widgets (buttons, text, sliders, scroll views, etc.)
- **Platform abstraction** - Customizable renderer and platform backends via interfaces
- **C++20 ready** - Uses modern C++ features (lambdas, concepts, etc.)
- **Header-only friendly** - Minimal dependencies (requires only EASTL)

## Architecture

```
ZSlate/
├── Core/          # Types, geometry, math utilities
├── Application/   # SlateApplication, input routing
├── Backend/       # ISlateRenderer interface
├── Widgets/
│   ├── Text/      # STextBlock, SRichTextBlock, SMultiLineEditableText
│   ├── Input/     # SButton, SCheckBox, SSlider, SEditableTextBox
│   ├── Layout/    # SBoxPanel, SScrollBox, SSplitter, SSpacer
│   ├── Views/     # SListView
│   ├── Panels/    # SBorder, SOverlay, SImage
│   ├── SWidget.h  # Base widget class
│   ├── SLeafWidget.h
│   └── SCompoundWidget.h
└── Platform/      # Platform-specific implementations
```

## Quick Start

### Include in your project

```cpp
#include "ZSlate/Application/SlateApplication.h"
#include "ZSlate/Core/SlatePaint.h"
#include "ZSlate/Widgets/Input/SButton.h"
#include "ZSlate/Widgets/Text/STextBlock.h"
```

### Implement ISlateRenderer

```cpp
struct MyRenderer : public ZSlate::ISlateRenderer
{
    void drawQuad(const UIRect& rect, const UIColor& color) override { /* ... */ }
    void drawRect(const UIRect& rect, const UIColor& color, float thickness = 1.0f) override { /* ... */ }
    void drawText(const UIRect& rect, const std::string& text, float font_size, const UIColor& color, ...) override { /* ... */ }
    // ... other methods
};
```

### Build UI

```cpp
// 1. Set platform
MyRenderer renderer;
MyPlatform platform{&renderer};
ZSlate::SetPlatform(&platform);

// 2. Build widget tree
auto root = std::make_shared<ZSlate::SVerticalBox>();

auto title = std::make_shared<ZSlate::STextBlock>();
title->Text = "Hello ZSlate!";
title->FontSize = 24.0f;
root->AddSlot(title);

auto button = std::make_shared<ZSlate::SButton>();
button->SetText("Click Me");
button->OnClicked.AddLambda([]() { /* handle click */ });
root->AddSlot(button);

// 3. Paint
ZSlate::SlateApplication::Get().SetRootContent(root);
ZSlate::SlateApplication::Get().PaintInto(&renderer, UIRect(0, 0, 800, 600));
```

## CMake Integration

```cmake
# Build from source
add_subdirectory(${ZSLATE_PATH}/ZSlate ${CMAKE_CURRENT_BINARY_DIR}/ZSlate)

target_link_libraries(MyApp PRIVATE ZSlate)
```

## Widget Reference

### Text Widgets (`Widgets/Text/`)

| Widget | Description |
|--------|-------------|
| `STextBlock` | Single line text display |
| `SRichTextBlock` | **Rich text display** with inline formatting (bold, color, links, etc.) |
| `SMultiLineEditableText` | **Multi-line text editor** with selection, clipboard, scroll |

### Input Widgets (`Widgets/Input/`)

| Widget | Description |
|--------|-------------|
| `SButton` | Clickable button |
| `SCheckBox` | Check box toggle |
| `SSlider` | Slider control |
| `SEditableTextBox` | Single-line text input |

### Layout Widgets (`Widgets/Layout/`)

| Widget | Description |
|--------|-------------|
| `SBoxPanel` | Horizontal/Vertical box layout |
| `SScrollBox` | Scrollable container |
| `SSplitter` | Resizable splitter |
| `SSpacer` | Empty space |

### Views Widgets (`Widgets/Views/`)

| Widget | Description |
|--------|-------------|
| `SListView<T>` | **Virtualized list view** (supports 1000+ items efficiently) |

### Panels Widgets (`Widgets/Panels/`)

| Widget | Description |
|--------|-------------|
| `SBorder` | Decorative border container |
| `SOverlay` | Z-ordered overlay (Z-Stack) |
| `SImage` | Image display |

### Other Widgets

| Widget | Description |
|--------|-------------|
| `SWidget` | Base widget class |
| `SLeafWidget` | Leaf widget base (no children) |
| `SCompoundWidget` | Single-child wrapper |
| `SMenu` | Menu system |

### SMultiLineEditableText Example

```cpp
#include "ZSlate/Widgets/Text/SMultiLineEditableText.h"

auto TextEditor = std::make_shared<ZSlate::SMultiLineEditableText>(ZSlate::SMultiLineEditableText::FTextOptions{
    .FontSize = 14.0f,
    .LineSpacing = 1.2f,
    .AutoWrap = true,
    .HintText = "Enter text here..."
});

TextEditor->SetText("Hello, this is a multi-line text editor.\nYou can type here!");
TextEditor->SetOnTextChanged([](const std::string& NewText) {
    std::cout << "Text changed: " << NewText.size() << " chars" << std::endl;
});
```

### SListView Example

```cpp
#include "ZSlate/Widgets/Views/SListView.h"
#include "ZSlate/Widgets/Layout/SHorizontalBox.h"
#include "ZSlate/Widgets/Text/STextBlock.h"

struct FMyItem { std::string Name; int32 Value; };

std::vector<FMyItem> Items;
for (int i = 0; i < 1000; ++i)
    Items.push_back({"Item " + std::to_string(i), i});

auto ListView = ZSlate::CreateListView<FMyItem>(
    std::move(Items),
    [](const FMyItem& Item, int32 Index, const std::shared_ptr<ZSlate::SListView<FMyItem>>& L) {
        auto Row = std::make_shared<ZSlate::SHorizontalBox>();
        auto IndexText = std::make_shared<ZSlate::STextBlock>();
        IndexText->Text = "[" + std::to_string(Index) + "] ";
        IndexText->FontSize = 14.0f;
        Row->AddSlot(IndexText);
        
        auto NameText = std::make_shared<ZSlate::STextBlock>();
        NameText->Text = Item.Name;
        NameText->FontSize = 14.0f;
        Row->AddSlot(NameText);
        return Row;
    },
    ZSlate::SListView<FMyItem>::FListViewOptions{.ItemHeight = 24.0f}
);
```

### SRichTextBlock Example

```cpp
#include "ZSlate/Widgets/Text/SRichTextBlock.h"

auto RichText = ZSlate::CreateRichTextBlock(
    "This is <b>bold</b>, <i>italic</i>, <u>underline</u>, "
    "and <color=#FF0000>red text</color>.\n"
    "Visit <Hyperlink url=\"https://example.com\">our website</Hyperlink>.",
    ZSlate::SRichTextBlock::FRichTextOptions{
        .FontSize = 14.0f,
        .Color = ZSlate::Colors::White,
        .AutoWrapText = true
    }
);

// Handle hyperlink clicks
RichText->SetOnHyperlinkNavigate([](const std::string& Url) {
    std::cout << "Navigating to: " << Url << std::endl;
});
```

**Supported Tags:**
- `<b>`, `<strong>` - Bold text
- `<i>`, `<em>` - Italic text
- `<u>` - Underline
- `<s>`, `<strike>` - Strikethrough
- `<color=#RRGGBB>` - Text color
- `<size=N>` - Font size
- `<Hyperlink url="...">text</>` - Clickable link
- `<br>` - Line break

## Type Reference

```cpp
// Vector2 - 2D vector
struct Vector2 { float x, y; };

// UIRect - Rectangle (x, y, w, h)
struct UIRect { float x, y, w, h; };

// UIColor - RGBA color (0.0-1.0)
using UIColor = Vector4;
namespace Colors { constexpr UIColor White, Black, Red, ... }
```

## Platform Implementations

- **ZEngine** - GPU-accelerated rendering via `ZEngineSlateRenderer` (see `platform/ZEngine/`)
- **MockRenderer** - Console output for testing (see `examples/`)

## License

See the project LICENSE for details.
