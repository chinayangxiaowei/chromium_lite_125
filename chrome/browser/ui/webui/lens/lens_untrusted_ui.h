// Copyright 2022 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_LENS_LENS_UNTRUSTED_UI_H_
#define CHROME_BROWSER_UI_WEBUI_LENS_LENS_UNTRUSTED_UI_H_

#include "base/memory/weak_ptr.h"
#include "chrome/browser/lens/core/mojom/lens.mojom.h"
#include "chrome/browser/ui/webui/top_chrome/untrusted_top_chrome_web_ui_controller.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_ui_data_source.h"
#include "ui/webui/resources/cr_components/color_change_listener/color_change_listener.mojom.h"
#include "ui/webui/resources/cr_components/searchbox/searchbox.mojom-forward.h"

namespace ui {
class ColorChangeHandler;
}

namespace lens {
class LensPageHandler;

// WebUI controller for the chrome-untrusted://lens page.
class LensUntrustedUI : public UntrustedTopChromeWebUIController,
                        public lens::mojom::LensPageHandlerFactory {
 public:
  explicit LensUntrustedUI(content::WebUI* web_ui);

  LensUntrustedUI(const LensUntrustedUI&) = delete;
  LensUntrustedUI& operator=(const LensUntrustedUI&) = delete;
  ~LensUntrustedUI() override;

  // Instantiates the implementor of the mojom::PageHandlerFactory mojo
  // interface passing the pending receiver that will be internally bound.
  void BindInterface(
      mojo::PendingReceiver<lens::mojom::LensPageHandlerFactory> receiver);

  // Instantiates the implementor of the searchbox::mojom::PageHandler mojo
  // interface passing the pending receiver that will be internally bound.
  void BindInterface(
      mojo::PendingReceiver<searchbox::mojom::PageHandler> receiver);

  // Instantiates the implementor of the
  // color_change_listener::mojom::PageHandler mojo interface passing the
  // pending receiver that will be internally bound.
  void BindInterface(
      mojo::PendingReceiver<color_change_listener::mojom::PageHandler>
          receiver);

  static constexpr std::string GetWebUIName() { return "LensUntrusted"; }

 private:
  // lens::mojom::LensPageHandlerFactory:
  void CreatePageHandler(
      mojo::PendingReceiver<lens::mojom::LensPageHandler> receiver,
      mojo::PendingRemote<lens::mojom::LensPage> page) override;
  void CreateSidePanelPageHandler(
      mojo::PendingReceiver<lens::mojom::LensSidePanelPageHandler> receiver,
      mojo::PendingRemote<lens::mojom::LensSidePanelPage> page) override;

  void LoadScreenshot(
      const std::string& resource_path,
      content::WebUIDataSource::GotDataCallback got_data_callback);

  std::unique_ptr<ui::ColorChangeHandler> color_provider_handler_;

  mojo::Receiver<lens::mojom::LensPageHandlerFactory>
      lens_page_factory_receiver_{this};

  base::WeakPtrFactory<LensUntrustedUI> weak_factory_{this};

  WEB_UI_CONTROLLER_TYPE_DECL();
};

}  // namespace lens
#endif  // CHROME_BROWSER_UI_WEBUI_LENS_LENS_UNTRUSTED_UI_H_
