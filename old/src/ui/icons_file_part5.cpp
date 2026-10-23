#include <ui/icons.hpp>
#include "icons_lookup.hpp"

namespace Icons
{
    const wchar_t *for_file_part5([[maybe_unused]] const std::string &name,
                                 [[maybe_unused]] const std::string &ext)
    {
        if (ends_with(name, ".test.js") || ends_with(name, ".test.jsx") || ends_with(name, ".test.ts") ||
            ends_with(name, ".test.tsx") || ends_with(name, ".spec.js") || ends_with(name, ".spec.jsx") ||
            ends_with(name, ".spec.ts") || ends_with(name, ".spec.tsx") || ends_with(name, "_test.go") ||
            ends_with(name, "_test.py") || ends_with(name, ".test.py") || ends_with(name, "_spec.rb") ||
            ends_with(name, ".spec.rb"))
            return lang_test;
        if (ends_with(name, ".snap"))
            return lang_snapshot;
        if (ext == ".txt" || ext == ".text" || ext == ".nfo" || ext == ".diz")
            return file_text;
        if (ext == ".map" || ext == ".min.js" || ext == ".min.css")
            return file_code;
        if (ext == ".wasm.map")
            return lang_wasm;

        // compound / double extensions via name
        if (ends_with(name, ".d.ts"))
            return lang_ts;
        if (ends_with(name, ".test.js") || ends_with(name, ".spec.js"))
            return lang_test;
        if (ends_with(name, ".module.css") || ends_with(name, ".module.scss"))
            return lang_css;
        if (ends_with(name, ".config.js") || ends_with(name, ".config.ts") ||
            ends_with(name, ".config.mjs") || ends_with(name, ".config.cjs"))
            return lang_config;
        if (ends_with(name, ".service.ts") || ends_with(name, ".controller.ts") ||
            ends_with(name, ".module.ts") || ends_with(name, ".guard.ts") ||
            ends_with(name, ".interceptor.ts") || ends_with(name, ".pipe.ts") ||
            ends_with(name, ".filter.ts") || ends_with(name, ".middleware.ts"))
            return lang_ts;
        if (ends_with(name, ".component.ts") || ends_with(name, ".component.js"))
            return lang_angular;
        if (ends_with(name, ".vue.ts") || ends_with(name, ".vue.js"))
            return lang_vue;
        return nullptr;
    }
}
