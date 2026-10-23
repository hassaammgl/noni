#include <configs/config.hpp>

void config_parse_features(AppConfig &cfg, const MiniJson::Value &root)
{
    if (const MiniJson::Value *v = root.get("formatOnSave"); v && v->is_bool())
        cfg.format_on_save = v->as_bool(cfg.format_on_save);

    if (const MiniJson::Value *clip = root.get("clipboard"); clip && clip->is_object())
    {
        if (const MiniJson::Value *s = clip->get("sync"); s && s->is_bool())
            cfg.clipboard.sync = s->as_bool(cfg.clipboard.sync);
    }

    if (const MiniJson::Value *as = root.get("autosave"); as && as->is_object())
    {
        cfg.autosave.mode = as->get_string("mode", cfg.autosave.mode);
        cfg.autosave.delay_ms = as->get_int("delayMs", cfg.autosave.delay_ms);
        if (cfg.autosave.delay_ms < 100)
            cfg.autosave.delay_ms = 100;
    }
}
