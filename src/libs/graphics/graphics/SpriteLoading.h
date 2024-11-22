#pragma once


#include "Sprite.h"

#include <renderer/Texture.h>
#include <renderer/TextureUtilities.h>


namespace ad {
namespace graphics {
namespace sprite {


/// Currently only the texture, but might be extended to contain the buffer of sprite rectangles
/// (so renderer could expect an index into the buffer, instead of complete sprite areas with 4 values)
struct LoadedAtlas
{
    std::shared_ptr<Texture> texture;
};


using SheetLoad = std::pair<LoadedAtlas, std::vector<LoadedSprite>>;
using SingleLoad = std::pair<LoadedAtlas, LoadedSprite>;


/// \attention This signature is not stable
template <class T_pixel>
LoadedAtlas loadAtlas(const arte::Image<T_pixel> & aRasterData);


/// \brief Takes a pair of iterator to SpriteArea instances, and the corresponding raster data
template <class T_iterator, class T_pixel>
SheetLoad load(T_iterator aFirst, T_iterator aLast, const arte::Image<T_pixel> & aRasterData);


/// \brief Overload accepting a range.
template <std::ranges::range T_range, class T_pixel>
SheetLoad load(T_range aRange, const arte::Image<T_pixel> & aRasterData);


/// \brief Load all sprites from a TileSheet.
SheetLoad load(const arte::TileSheet & aTileSheet);


/// \brief Load all sprites defined in the meta (tilesheet) file.
SheetLoad loadMetaFile(const filesystem::path & aPath);


/// \brief Load the entire image as a single sprite.
template <class T_pixel>
SingleLoad load(const arte::Image<T_pixel> & aRasterData);


//
// Implementations
//
template <class T_pixel>
LoadedAtlas loadAtlas(const arte::Image<T_pixel> & aRasterData)
{
    LoadedAtlas atlas{.texture{std::make_shared<Texture>(GL_TEXTURE_RECTANGLE)}};
    loadImage(*atlas.texture, aRasterData);
    setFiltering(*atlas.texture, GL_NEAREST);
    return atlas;
}


template <class T_iterator, class T_pixel>
SheetLoad load(T_iterator aFirst, T_iterator aLast, const arte::Image<T_pixel> & aRasterData)
{
    static_assert(std::is_convertible_v<decltype(*std::declval<T_iterator>()), SpriteArea>,
                  "Iterators must point to SpriteArea instances.");

    std::vector<LoadedSprite> loadedSprites;
    std::copy(aFirst, aLast, std::back_inserter(loadedSprites));
    return {loadAtlas(aRasterData), loadedSprites};
}


template <std::ranges::range T_range, class T_pixel>
SheetLoad load(T_range aRange, const arte::Image<T_pixel> & aRasterData)
{
    return load(std::begin(aRange), std::end(aRange), aRasterData);
}


template <class T_pixel>
SingleLoad load(const arte::Image<T_pixel> & aRasterData)
{
    std::array<SpriteArea, 1> fullSize{ SpriteArea{{0, 0}, aRasterData.dimensions()} };
    LoadedAtlas atlas;
    std::vector<LoadedSprite> sprites;
    std::tie(atlas, sprites) = load(fullSize, aRasterData);
    return {atlas, sprites.front()};
}


} // namespace sprite
} // namespace graphics
} // namespace ad
