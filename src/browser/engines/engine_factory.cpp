#include "browser/engines/engine_factory.h"
#include "browser/webengine_compat.h"

#if SEB_HAS_QTWEBENGINE
#include "browser/engines/qtwebengine/qt_webengine_provider.h"
#elif SEB_HAS_WEBKITGTK
#include "browser/engines/webkitgtk/webkitgtk_provider.h"
#endif

namespace seb::browser {

std::unique_ptr<contracts::IEngineProvider> createEngineProvider()
{
#if SEB_HAS_QTWEBENGINE
    return std::make_unique<QtWebEngineProvider>();
#elif SEB_HAS_WEBKITGTK
    return std::make_unique<WebKitGtkProvider>();
#else
    return nullptr;
#endif
}

}  // namespace seb::browser
