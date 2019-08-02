STEAM_API_FLAT=extern/steamworks/public/steam/steam_api_flat.h
STEAM_API_COMMON=extern/steamworks/public/steam/steam_api_common.h
ISTEAMUTILS=extern/steamworks/public/steam/isteamutils.h

sed -i '/typedef int64 lint64;\|typedef uint64 ulint64;/d' $STEAM_API_FLAT
sed -i 's/\(^[a-zA-Z].* const_k_\)/\/\/ \1/g' $STEAM_API_FLAT

#Version 1.46:
sed -i 's/\(^.*ESteamTVRegionBehavior\)/\/\/ \1/g' $STEAM_API_FLAT

# #define __cdecl gives unnecessary warnings
if [[ "$OSTYPE" == "msys" ]]; then
    sed -i 's/\(^#define __cdecl\)/\/\/ \1/g' $STEAM_API_COMMON
    sed -i 's/\(^#define __cdecl\)/\/\/ \1/g' $ISTEAMUTILS
fi
