#include <ui/icons.hpp>
#include "icons_lookup.hpp"

namespace Icons
{
    const wchar_t *for_file_part4([[maybe_unused]] const std::string &name,
                                 [[maybe_unused]] const std::string &ext)
    {
        if (ext == ".rtf")
            return lang_rtf;
        if (ext == ".pages")
            return lang_pages;
        if (ext == ".numbers")
            return lang_numbers;
        if (ext == ".key")
            return lang_keynote;
        if (ext == ".pdf")
            return file_pdf;
        if (ext == ".epub")
            return lang_epub;
        if (ext == ".mobi" || ext == ".azw" || ext == ".azw3")
            return lang_mobi;
        if (ext == ".fb2" || ext == ".djvu" || ext == ".chm" || ext == ".cbr" || ext == ".cbz")
            return lang_ebook;
        if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".jpe" || ext == ".jfif" ||
            ext == ".gif" || ext == ".webp" || ext == ".bmp" || ext == ".dib" || ext == ".pbm" ||
            ext == ".pgm" || ext == ".ppm" || ext == ".pnm" || ext == ".xpm" || ext == ".xbm")
            return lang_image;
        if (ext == ".ico" || ext == ".icns" || ext == ".cur" || ext == ".ani")
            return lang_ico;
        if (ext == ".tif" || ext == ".tiff")
            return lang_tiff;
        if (ext == ".heic" || ext == ".heif" || ext == ".avif" || ext == ".jxl")
            return lang_heic;
        if (ext == ".raw" || ext == ".cr2" || ext == ".nef" || ext == ".arw" || ext == ".dng" ||
            ext == ".orf" || ext == ".rw2")
            return lang_raw;
        if (ext == ".exr" || ext == ".hdr" || ext == ".pic")
            return lang_exr;
        if (ext == ".psd" || ext == ".psb" || ext == ".ai" || ext == ".eps" || ext == ".indd" ||
            ext == ".sketch" || ext == ".fig" || ext == ".xd" || ext == ".afdesign" || ext == ".afphoto")
            return lang_design;
        if (ext == ".ttf" || ext == ".otf" || ext == ".woff" || ext == ".woff2" || ext == ".eot" ||
            ext == ".fon" || ext == ".fnt" || ext == ".pfb" || ext == ".pfm")
            return lang_font;
        if (ext == ".mp3" || ext == ".wav" || ext == ".flac" || ext == ".ogg" || ext == ".oga" ||
            ext == ".opus" || ext == ".aac" || ext == ".m4a" || ext == ".wma" || ext == ".aiff" ||
            ext == ".aif" || ext == ".ape" || ext == ".alac")
            return lang_audio;
        if (ext == ".mid" || ext == ".midi")
            return lang_midi;
        if (ext == ".mod" || ext == ".xm" || ext == ".it" || ext == ".s3m")
            return lang_mod;
        if (ext == ".mp4" || ext == ".m4v" || ext == ".mkv" || ext == ".webm" || ext == ".avi" ||
            ext == ".mov" || ext == ".wmv" || ext == ".flv" || ext == ".mpeg" || ext == ".mpg" ||
            ext == ".mpe" || ext == ".3gp" || ext == ".3g2" || ext == ".ogv" || ext == ".mts" ||
            ext == ".m2ts")
            return lang_video;
        if (ext == ".srt" || ext == ".vtt" || ext == ".ass" || ext == ".ssa" || ext == ".sub" ||
            ext == ".idx" || ext == ".smi")
            return lang_subtitle;
        if (ext == ".m3u" || ext == ".m3u8" || ext == ".pls" || ext == ".asx" || ext == ".xspf")
            return lang_playlist;
        if (ext == ".cue")
            return lang_cue;
        if (ext == ".zip" || ext == ".gz" || ext == ".tgz" || ext == ".bz2" || ext == ".tbz2" ||
            ext == ".xz" || ext == ".txz" || ext == ".lz" || ext == ".lzma" || ext == ".zst" ||
            ext == ".zstd" || ext == ".tar" || ext == ".7z" || ext == ".rar" || ext == ".cab" ||
            ext == ".iso" || ext == ".img" || ext == ".dmg" || ext == ".pkg" || ext == ".apk" ||
            ext == ".ipa" || ext == ".deb" || ext == ".rpm" || ext == ".msi" || ext == ".appimage" ||
            ext == ".snap" || ext == ".flatpak")
            return file_zip;
        if (ext == ".jar" || ext == ".war" || ext == ".ear" || ext == ".aar")
            return lang_jar;
        if (ext == ".nupkg" || ext == ".snupkg")
            return lang_nupkg;
        if (ext == ".whl" || ext == ".egg")
            return lang_package;
        if (ext == ".gem")
            return lang_gemfile;
        if (ext == ".crate")
            return lang_cargo;
        if (ext == ".lock")
            return lang_lock;
        if (ext == ".pem" || ext == ".crt" || ext == ".cer" || ext == ".der" || ext == ".p12" ||
            ext == ".pfx" || ext == ".p7b" || ext == ".p7c" || ext == ".csr")
            return lang_certificate;
        if (ext == ".key" || ext == ".pub" || ext == ".gpg" || ext == ".asc" || ext == ".sig" ||
            ext == ".sign")
            return lang_key;
        if (ext == ".kdbx" || ext == ".kdb")
            return lang_lock;
        if (ext == ".log")
            return lang_log;
        if (ext == ".tmp" || ext == ".temp" || ext == ".swp" || ext == ".swo" || ext == ".bak" ||
            ext == ".old" || ext == ".orig" || ext == ".rej")
            return lang_temp;
        if (ext == ".o" || ext == ".obj" || ext == ".a" || ext == ".lib" || ext == ".so" ||
            ext == ".dylib" || ext == ".dll" || ext == ".exe" || ext == ".bin" || ext == ".elf" ||
            ext == ".ko" || ext == ".out" || ext == ".app" || ext == ".com")
            return file_binary;
        if (ext == ".class" || ext == ".beam" || ext == ".hi")
            return lang_bytecode;
        if (ext == ".sln")
            return lang_sln;
        if (ext == ".csproj")
            return lang_csproj;
        if (ext == ".fsproj")
            return lang_fsproj;
        if (ext == ".vbproj")
            return lang_vbproj;
        if (ext == ".vcxproj" || ext == ".vcproj")
            return lang_vcxproj;
        if (ext == ".filters" || ext == ".props" || ext == ".targets" || ext == ".nuspec")
            return lang_props;
        if (ext == ".xcodeproj" || ext == ".xcworkspace" || ext == ".pbxproj" || ext == ".storyboard" ||
            ext == ".xib" || ext == ".plist")
            return lang_xcode;
        if (ext == ".desktop")
            return lang_desktop;
        if (ext == ".service" || ext == ".socket" || ext == ".timer" || ext == ".target" ||
            ext == ".mount" || ext == ".path" || ext == ".slice" || ext == ".scope")
            return lang_service;
        if (ext == ".1" || ext == ".2" || ext == ".3" || ext == ".4" || ext == ".5" || ext == ".6" ||
            ext == ".7" || ext == ".8" || ext == ".9" || ext == ".man" || ext == ".mdoc")
            return lang_man;
        if (ext == ".info")
            return lang_info;
        if (ext == ".puml" || ext == ".plantuml" || ext == ".pu")
            return lang_plantuml;
        if (ext == ".mmd" || ext == ".mermaid")
            return lang_mermaid;
        if (ext == ".dot" || ext == ".gv")
            return lang_dot;
        if (ext == ".drawio" || ext == ".dio")
            return lang_drawio;
        if (ext == ".ics" || ext == ".ical" || ext == ".ifb")
            return lang_calendar;
        if (ext == ".vcf" || ext == ".vcard")
            return lang_contact;
        if (ext == ".rss" || ext == ".atom")
            return lang_rss;
        if (ext == ".torrent")
            return lang_torrent;
        if (ext == ".iso" || ext == ".img" || ext == ".vhd" || ext == ".vmdk" || ext == ".qcow2")
            return lang_iso;
        if (ext == ".parquet" || ext == ".avro" || ext == ".orc" || ext == ".arrow" || ext == ".feather")
            return lang_parquet;
        if (ext == ".h5" || ext == ".hdf5" || ext == ".hdf")
            return lang_hdf5;
        if (ext == ".fbs")
            return lang_flatbuffers;
        if (ext == ".graphqls")
            return lang_graphql_sdl;
        if (ext == ".story.js" || ext == ".story.jsx" || ext == ".story.ts" || ext == ".story.tsx" ||
            ends_with(name, ".stories.js") || ends_with(name, ".stories.jsx") ||
            ends_with(name, ".stories.ts") || ends_with(name, ".stories.tsx"))
            return lang_story;
        return nullptr;
    }
}
