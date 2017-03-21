#include "etc.h"
#include "sharedstate.h"
#include "binding-util.h"
#include "binding-types.h"
#include "config.h"

static bool isCached = false;

#ifdef _WIN32
#include <windows.h>
static WCHAR szStyle[8] = {0};
static WCHAR szTile[8] = {0};
static WCHAR szFile[MAX_PATH+1] = {0};
static DWORD oldcolor = 0;
static DWORD szStyleSize = sizeof(szStyle) - 1;
static DWORD szTileSize = sizeof(szTile) - 1;
static bool setStyle = false;
static bool setTile = false;
#elif __linux__
#include <iostream>
#include <iomanip>
#include <sstream>
#include <gio/gio.h>
static GVariant *oldPictureURI;
static GVariant *oldPictureOptions;
static GVariant *oldPrimaryColor;
static GVariant *oldColorShadingType;
#endif

RB_METHOD(wallpaperSet)
{
	RB_UNUSED_PARAM;
	const char *imageName;
	int color;
	rb_get_args(argc, argv, "zi", &imageName, &color RB_ARG_END);
	std::string imgname = shState->config().gameFolder + "/Wallpaper/" + imageName + ".bmp";
#ifdef _WIN32
	// Crapify the slashes
	size_t index = 0;
	for (;;) {
		index = imgname.find("/", index);
		if (index == std::string::npos)
			break;
		imgname.replace(index, 1, "\\");
		index += 1;
	}
	WCHAR imgnameW[MAX_PATH];
	WCHAR imgnameFull[MAX_PATH];
	MultiByteToWideChar(CP_UTF8, 0, imgname.c_str(), -1, imgnameW, MAX_PATH);
	GetFullPathNameW(imgnameW, MAX_PATH, imgnameFull, NULL);


	int colorId = COLOR_BACKGROUND;
	WCHAR zero[2] = L"0";
	DWORD zeroSize = 4;

	HKEY hKey = NULL;
	if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Control Panel\\Desktop", 0, KEY_READ, &hKey) != ERROR_SUCCESS)
		goto end;

	if (!isCached) {
		// QUERY

		// Style
		setStyle = RegQueryValueExW(hKey, L"WallpaperStyle", 0, NULL, (LPBYTE)(szStyle), &szStyleSize) == ERROR_SUCCESS;

		// Tile
		setTile = RegQueryValueExW(hKey, L"TileWallpaper", 0, NULL, (LPBYTE)(szTile), &szTileSize) == ERROR_SUCCESS;

		// File path
		if (!SystemParametersInfoW(SPI_GETDESKWALLPAPER, MAX_PATH, (PVOID)szFile, 0))
			goto end;

		// Color
		oldcolor = GetSysColor(COLOR_BACKGROUND);

		isCached = true;
	}

	RegCloseKey(hKey);
	hKey = NULL;
	if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Control Panel\\Desktop", 0, KEY_WRITE, &hKey) != ERROR_SUCCESS)
		goto end;

	// SET

	// Set the style
	if (RegSetValueExW(hKey, L"WallpaperStyle", 0, REG_SZ, (const BYTE*)zero, zeroSize) != ERROR_SUCCESS)
		goto end;

	if (RegSetValueExW(hKey, L"TileWallpaper", 0, REG_SZ, (const BYTE*)zero, zeroSize) != ERROR_SUCCESS)
		goto end;

	// Set the wallpaper
	if (!SystemParametersInfoW(SPI_SETDESKWALLPAPER, 0, (PVOID)imgnameFull, SPIF_UPDATEINIFILE))
		goto end;

	// Set the color
	if (!SetSysColors(1, &colorId, (const COLORREF *)&color))
		goto end;
end:
	if (hKey)
		RegCloseKey(hKey);
#elif __linux__
    // GNOME 3 and related window managers
    GSettings *settings;

    settings = g_settings_new ("org.gnome.desktop.background");

    if (!isCached) {
        oldPictureURI = g_settings_get_value(settings, "picture-uri");
        oldPictureOptions = g_settings_get_value(settings, "picture-options");
        oldPrimaryColor = g_settings_get_value(settings, "primary-color");
        oldColorShadingType = g_settings_get_value(settings, "color-shading-type");

        isCached = true;
    }
    
    imgname.insert(0, 1, 'file://');
    
    g_settings_set_value(settings, "picture-uri", g_variant_new ("s", imgname.c_str()));
    g_settings_set_value(settings, "picture-options", g_variant_new ("s", "centered"));    
    
    std::stringstream colorStream;
    colorStream << "#" << std::setfill ('0') << std::hex << color;

    std::cout << colorStream.str();
    std::cout << colorStream.str();
    std::cout.flush();

    g_settings_set_value(settings, "primary-color", g_variant_new ("s", colorStream.str().c_str()));
    g_settings_set_value(settings, "color-shading-type", g_variant_new ("s", "solid"));

    g_object_unref(settings);
#endif
	return Qnil;
}

RB_METHOD(wallpaperReset)
{
	RB_UNUSED_PARAM;
#ifdef _WIN32
	if (isCached) {
		int colorId = COLOR_BACKGROUND;
		HKEY hKey = NULL;
		if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Control Panel\\Desktop", 0, KEY_WRITE, &hKey) != ERROR_SUCCESS)
			goto end;

		// Set the style
		if (setStyle)
			RegSetValueExW(hKey, L"WallpaperStyle", 0, REG_SZ, (const BYTE*)szStyle, szStyleSize);

		if (setTile)
			RegSetValueExW(hKey, L"TileWallpaper", 0, REG_SZ, (const BYTE*)szTile, szTileSize);

		// Set the wallpaper
		if (!SystemParametersInfoW(SPI_SETDESKWALLPAPER, 0, (PVOID)szFile, SPIF_UPDATEINIFILE))
			goto end;

		// Set the color
		if (!SetSysColors(1, &colorId, (const COLORREF *)&oldcolor))
			goto end;
	end:
		if (hKey)
			RegCloseKey(hKey);
	}
#elif __linux__
    if (isCached) {
        // GNOME 3 and related window managers
        GSettings *settings;

        settings = g_settings_new ("org.gnome.desktop.background");

        g_settings_set_value(settings, "picture-uri", oldPictureURI);
        g_settings_set_value(settings, "picture-options", oldPictureOptions);

        g_settings_set_value(settings, "primary-color", oldPrimaryColor);
        g_settings_set_value(settings, "color-shading-type", oldColorShadingType);

        g_object_unref(settings);
    }
#endif
	return Qnil;
}

void wallpaperBindingInit()
{
    VALUE module = rb_define_module("Wallpaper");

    //Functions
    _rb_define_module_function(module, "set", wallpaperSet);
    _rb_define_module_function(module, "reset", wallpaperReset);
}
