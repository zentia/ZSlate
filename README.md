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
├── Widgets/       # 20+ UI widgets
├── Backend/       # ISlateRenderer interface
└── Platform/      # Platform-specific implementations
```

## Quick Start

### Include in your project

```cpp
#include "ZSlate/Application/SlateApplication.h"
#include "ZSlate/Core/SlatePaint.h"
#include "ZSlate/Widgets/SButton.h"
#include "ZSlate/Widgets/STextBlock.h"
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

| Widget | Description |
|--------|-------------|
| `SWidget` | Base widget class |
| `SVerticalBox` | Vertical layout container |
| `SHorizontalBox` | Horizontal layout container |
| `SButton` | Clickable button |
| `STextBlock` | Text display |
| `SSpacer` | Empty space |
| `SScrollBox` | Scrollable container |
| `SListView<T>` | **Virtualized list view** (supports 1000+ items efficiently) |
| `SBorder` | Decorative border |
| `SOverlay` | Z-ordered overlay (Z-Stack) |
| `SSplitter` | Resizable splitter |
| `SCheckBox` | Check box |
| `SSlider` | Slider control |
| `SEditableTextBox` | Text input field |
| `SMenu` | Menu system |

### SListView Example

```cpp
#include "ZSlate/Widgets/SListView.h"

// Define your item type
struct FMyItem
{
    std::string Name;
    int32 Value;
};

// Create list view
std::vector<FMyItem> Items;
for (int i = 0; i < 1000; ++i)
{
    Items.push_back({"Item " + std::to_string(i), i});
}

auto ListView = ZSlate::CreateListView<FMyItem>(
    std::move(Items),
    [](const FMyItem& Item, int32 Index, const std::shared_ptr<ZSlate::SListView<FMyItem>>& ListView) {
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
    ZSlate::SListView<FMyItem>::FListViewOptions{
        .ItemHeight = 24.0f,
        .ScrollBarColor = ZSlate::Colors::Gray
    }
);

// Set selection callback
ListView->SetOnSelectionChanged([](const std::vector<int32>& SelectedIndices) {
    if (!SelectedIndices.empty())
        std::cout << "Selected: " << SelectedIndices.front() << std::endl;
});

// Scroll to a specific item
ListView->ScrollToItem(500);
```

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
