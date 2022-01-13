#pragma once

#include <graphics/CameraUtilities.h>
#include <graphics/SpriteLoading.h>
#include <graphics/Tiling.h>

#include <resource/PathProvider.h>


namespace ad {
namespace graphics {


constexpr Size2<int> gCellSize{320, 180}; // The images resolution
constexpr double gScrollSpeed = 150.;

class Scene
{
public:
    Scene(Size2<int> aRenderResolution) :
        // +2 : 
        // * 1 for rounding up the division (the last tile, partially shown)
        // * 1 excess tile, i.e. initially completely "out of viewport"
        mGridSize{(gCellSize.width() / aRenderResolution.width()) + 2, 1},
        mTiling{},
        mTileSet{gCellSize, mGridSize}
    {
        setViewportVirtualResolution(mTiling, aRenderResolution, ViewOrigin::LowerLeft);

        sprites::LoadedAtlas atlas;
        std::tie(atlas, mLoadedTiles) = 
            sprites::load(arte::Image<math::sdr::Rgba>{
                    resource::pathFor("parallax/darkforest/DarkForest_Foreground.png"),
                    arte::ImageOrientation::InvertVerticalAxis});
        mTiling.load(atlas);

        std::ranges::fill(mPlacedTiles, mLoadedTiles.at(0));
        mTileSet.updateInstances(mPlacedTiles);
    }

    void update(double aTimePointSeconds)
    {
        Position2<GLfloat> gridPosition{
            (float)std::fmod(-aTimePointSeconds * gScrollSpeed, gCellSize.width()),
            0.f
        };

        mTiling.setPosition(mTileSet, gridPosition);
    }

    void render()
    {
        mTiling.render(mTileSet);
    }

private:
    Size2<int> mGridSize;
    Tiling mTiling;
    TileSet mTileSet;
    std::vector<LoadedSprite> mLoadedTiles; // The list of available tiles
    std::vector<TileSet::Instance> mPlacedTiles{mGridSize.area(), TileSet::gEmptyInstance};
};


} // namespace graphics
} // namespace ad
