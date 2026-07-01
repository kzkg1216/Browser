# MiniBrowser

Windows 向けの最小限 (MVP) の C++ ブラウザです。
Win32 API でウィンドウとツールバーを作り、レンダリングエンジンとして
**WebView2**(Microsoft Edge に使われている Chromium ベースのエンジン)を組み込んでいます。

## 機能 (MVP)

- Web ページの表示
- アドレスバー(Enter で移動、スキーム省略時は `https://` を補完)
- 戻る / 進む / 再読み込み ボタン
- ページタイトルのウィンドウタイトルへの反映
- High DPI 対応

## アーキテクチャ

```
+--------------------------------------------------+
| MiniBrowser (Win32 ウィンドウ)                    |
| +---+ +---+ +---+ +----------------------------+ |
| | ← | | → | | ↻ | | アドレスバー (EDIT)         | |
| +---+ +---+ +---+ +----------------------------+ |
| +----------------------------------------------+ |
| | WebView2 (Chromium/Blink エンジン)            | |
| |   HTML/CSS の描画・JavaScript の実行を担当    | |
| +----------------------------------------------+ |
+--------------------------------------------------+
```

- **自分で書く部分**: ウィンドウ・ツールバーなどの UI(`src/main.cpp`)
- **エンジン (WebView2)**: HTML/CSS/JS の処理はすべてこちらに任せる。
  Windows 10/11 には WebView2 ランタイムが標準搭載されているため、
  配布物は小さな exe と `WebView2Loader.dll` だけで済みます。

## ビルド方法

### 必要なもの

- Windows 10 / 11
- Visual Studio 2022(「C++ によるデスクトップ開発」ワークロード)
- CMake 3.20 以降(Visual Studio に同梱のものでも可)

WebView2 SDK は CMake の構成時に NuGet から自動ダウンロードされるため、
手動でのインストールは不要です。

### 手順

ターゲットは **x86** と **arm64** です。開発者コマンドプロンプトで:

```bat
:: x86 ビルド
cmake -B build-x86 -A Win32
cmake --build build-x86 --config Release

:: arm64 ビルド (x64 マシンからのクロスコンパイル可)
cmake -B build-arm64 -A ARM64
cmake --build build-arm64 --config Release
```

実行:

```bat
build-x86\Release\MiniBrowser.exe
```

### CI/CD

GitHub Actions(`.github/workflows/build.yml`)で push / PR ごとに
`windows-latest` ランナー上で x86 / arm64 の両方をビルドし、
exe と `WebView2Loader.dll` をアーティファクトとしてアップロードします。

## 今後の拡張候補

- タブ機能(WebView2 を複数持ち、切り替える)
- ブックマーク・履歴
- 検索語をアドレスバーに入れたら検索エンジンへ飛ばす
- ダウンロード UI、コンテキストメニューのカスタマイズ
- 新規ウィンドウ要求 (`add_NewWindowRequested`) のハンドリング
