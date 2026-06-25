# ZSlate

A lightweight, retained-mode UI framework for C++20 applications.

## Features

- **Retained-mode widget tree** — Declarative UI construction with automatic layout
- **Rich widget library** — 20+ built-in widgets (buttons, text, sliders, scroll views, etc.)
- **Batch renderer** — `ZSlateBatchedRenderer` records CPU draw commands that any GPU
  backend can consume
- **D3D11 renderer** — Optional `ZSlateD3D11Renderer` GPU backend (zero-config)
- **Font atlas** — TTF/OTC glyph rasterization via `ZSlateFontAtlas` + `ZSlateTextGenerator`
  (stb_truetype, on-demand baking, CJK fallback chain)
- **C++20 ready** — Uses modern C++ features (lambdas, etc.)
- **Header-friendly** — Minimal dependencies

## Architecture

```
ZSlate/
├── Core/            # Types (Vector2, UIRect, UIColor), geometry, enums
├── Application/     # SlateApplication, SlateInputRouter (mouse/keyboard)
├── Renderer/        # ZSlateBatchedRenderer (CPU batch recorder),
│                    # ZSlateFontAtlas (TTF→atlas), ZSlateTextGenerator (layout)
├── Platform/
│   └── D3D11/       # ZSlateD3D11Renderer (optional GPU backend)
├── Backend/         # ZEngineSlateRenderer (bridges to ZEngine runtime)
├── Widgets/
│   ├── Text/        # STextBlock, SRichTextBlock, SMultiLineEditableText
│   ├── Input/       # SButton, SCheckBox, SSlider, SEditableTextBox
│   ├── Layout/      # SBoxPanel, SScrollBox, SSplitter, SSpacer
│   ├── Views/       # SListView
│   ├── Panels/      # SBorder, SOverlay, SImage
│   ├── SWidget.h    # Base widget class
│   ├── SLeafWidget.h
│   └── SCompoundWidget.h
├── thirdparty/      # zstb_truetype.h (stb_truetype 1.26)
└── examples/        # Standalone demo apps
```

## Quick Start

### Option 1: Use D3D11 backend (recommended for tools)

```cpp
#include "ZSlate/Application/SlateApplication.h"
#include "ZSlate/Platform/D3D11/ZSlateD3D11Renderer.h"
#include "ZSlate/Renderer/ZSlateBatchedRenderer.h"
#include "ZSlate/Renderer/ZSlateFontAtlas.h"
#include "ZSlate/Widgets/Input/SButton.h"
#include "ZSlate/Widgets/Text/STextBlock.h"

// --- 1. Platform stub ---
struct MyPlatform : public ZSlate::ISlatePlatform
{
    ZSlate::ZSlateBatchedRenderer Renderer;
    ZSlate::Vector2 m_MousePos, m_WinSize {1024, 768};

    ZSlate::ISlateRenderer* GetRenderer()    override { return &Renderer; }
    ZSlate::Vector2 GetMousePosition() const override { return m_MousePos; }
    bool IsMouseButtonDown(int)         const override { return false; }
    bool IsKeyDown(int)                 const override { return false; }
    float GetTimeSeconds()              const override { return 0.0f; }
    ZSlate::Vector2 GetWindowSize()     const override { return m_WinSize; }
};

// --- 2. WinMain ---
int WINAPI WinMain(...)
{
    // Font atlas (text rendering)
    ZSlate::ZSlateFontAtlas fontAtlas;
    fontAtlas.LoadFromFile("C:\\Windows\\Fonts\\segoeui.ttf");
    fontAtlas.AddFallbackFont("C:\\Windows\\Fonts\\msyh.ttc");  // CJK

    MyPlatform platform;
    platform.Renderer.SetFontAtlas(&fontAtlas);
    ZSlate::SetPlatform(&platform);

    // Build UI
    auto root = std::make_shared<ZSlate::SVerticalBox>();
    auto title = std::make_shared<ZSlate::STextBlock>();
    title->Text = "Hello ZSlate!";
    root->AddSlot(title);

    ZSlate::SlateApplication::Get().SetRootContent(root);

    // Create window + D3D11 backend
    HWND hwnd = CreateWindowA(...);
    ZSlate::ZSlateD3D11Renderer backend(hwnd);
    backend.Init();

    // Main loop
    while (running) {
        platform.Renderer.BeginFrame();
        ZSlate::SlateApplication::Get().PaintInto(&platform.Renderer,
            ZSlate::UIRect(0, 0, 1024, 768));
        platform.Renderer.EndFrame();
        backend.Render(platform.Renderer, &fontAtlas);
    }

    backend.Shutdown();
}
```

### Option 2: Implement ISlateRenderer directly

```cpp
struct MyRenderer : public ZSlate::ISlateRenderer
{
    void DrawQuad(const UIRect& rect, const UIColor& color) override { /* ... */ }
    void DrawRect(const UIRect& rect, const UIColor& color, float t) override { /* ... */ }
    void DrawTexturedQuad(const UIRect& rect, void* tex, const UIColor& tint) override { /* ... */ }
    void DrawTextLabel(const UIRect& rect, const std::string& text, float size,
                       const UIColor& color, TextAnchor anchor, TextWrapMode wrap,
                       void* font_handle) override { /* ... */ }
    void DrawTextLabel(const std::string& text, const Vector2& pos, float size,
                       const UIColor& color) override { /* ... */ }
    Vector2 MeasureText(const std::string& text, float size) const override { /* ... */ }
    // ... PushClipRect, PopClipRect, BeginFrame, EndFrame
};
```

> **Note on naming**: `DrawTextLabel` was intentionally chosen over `DrawText` to avoid
> collision with the `windows.h` `DrawText` macro (which `#define`s `DrawText` →
> `DrawTextA`/`DrawTextW`).

## CMake Integration

```cmake
add_subdirectory(${ZSLATE_PATH}/ZSlate ${CMAKE_CURRENT_BINARY_DIR}/ZSlate)
target_link_libraries(MyApp PRIVATE ZSlate)
```

## Widget Reference

### Text Widgets (`Widgets/Text/`)

| Widget | Description |
|--------|-------------|
| `STextBlock` | Single line text display |
| `SRichTextBlock` | Rich text display with inline formatting (bold, color, links, etc.) |
| `SMultiLineEditableText` | Multi-line text editor with selection, clipboard, scroll |

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
| `SListView<T>` | Virtualized list view (supports 1000+ items efficiently) |

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

RichText->SetOnHyperlinkNavigate([](const std::string& Url) {
    std::cout << "Navigating to: " << Url << std::endl;
});
```

**Supported Tags:** `<b>`, `<strong>`, `<i>`, `<em>`, `<u>`, `<s>`, `<strike>`,
`<color=#RRGGBB>`, `<size=N>`, `<Hyperlink url="...">text</>`, `<br>`

### SListView Example

```cpp
#include "ZSlate/Widgets/Views/SListView.h"

struct FMyItem { std::string Name; int32 Value; };

std::vector<FMyItem> Items;
for (int i = 0; i < 1000; ++i)
    Items.push_back({"Item " + std::to_string(i), i});

auto ListView = ZSlate::CreateListView<FMyItem>(
    std::move(Items),
    [](const FMyItem& Item, int32 Index, auto& L) {
        auto Row = std::make_shared<ZSlate::SHorizontalBox>();
        auto NameText = std::make_shared<ZSlate::STextBlock>();
        NameText->Text = "[" + std::to_string(Index) + "] " + Item.Name;
        NameText->FontSize = 14.0f;
        Row->AddSlot(NameText);
        return Row;
    },
    ZSlate::SListView<FMyItem>::FListViewOptions{.ItemHeight = 24.0f}
);
```

## Type Reference

```cpp
// Vector2 — 2D vector
struct Vector2 { float x, y; };

// UIRect — Rectangle
struct UIRect { float x, y, w, h; };

// UIColor — RGBA color (0.0–1.0)
using UIColor = Vector4;
namespace Colors { constexpr UIColor White, Black, Red, ... };
```

## Font Atlas (ZSlateFontAtlas)

```cpp
#include "ZSlate/Renderer/ZSlateFontAtlas.h"

ZSlate::ZSlateFontAtlas atlas;
atlas.LoadFromFile("C:\\Windows\\Fonts\\segoeui.ttf");   // primary
atlas.AddFallbackFont("C:\\Windows\\Fonts\\msyh.ttc");    // CJK fallback

renderer.SetFontAtlas(&atlas);  // attach to batch renderer
// Font atlas auto-uploads to GPU via ZSlateD3D11Renderer::UploadFontAtlas()
```

## Platform Implementations

- **ZSlateD3D11Renderer** — D3D11 GPU backend, self-contained (no ZEngine dep)
- **ZEngineSlateRenderer** — Bridges to ZEngine's `BatchedUIRenderer` + UIPass
- **MockRenderer** — Console output for testing (see `examples/`)

## License

See the project LICENSE for details.
