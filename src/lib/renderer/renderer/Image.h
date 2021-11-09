#pragma once

#include "commons.h"

#include "stb_image.h"

#include <handy/Guard.h>

#include <memory>
#include <vector>

namespace ad {
namespace graphics {


inline stbi_uc * loadWithStbi(const std::string & aFilePath, Size2<int> & aDimension, int aSourceComponents)
{
    stbi_set_flip_vertically_on_load(true); // adapt to OpenGL
    return stbi_load(aFilePath.c_str(),
                     &aDimension.width(),
                     &aDimension.height(),
                     &aSourceComponents,
                     STBI_rgb_alpha /*equivalent to gComponents*/);
}


struct Image : public ResourceGuard<unsigned char *>
{
    Image(const std::string & aFilePath) :
        ResourceGuard<unsigned char*>(nullptr, stbi_image_free),
        mDimension(mDimension.Zero())
    {
        // It would be better to directly initialize on the resource guard constructor.
        // Yet stbi_load also assigns the dimension, and the mDimension member is initialized after 
        // the base ResourceGuard...
        resetResource(loadWithStbi(aFilePath, mDimension, mSourceComponents), stbi_image_free);
        if (get() == nullptr)
        {
            const std::string message = "Unable to load image from file '" + aFilePath + "'";
            std::cerr << message;
            throw std::runtime_error(message);
        }
    }

    /// \return First position after last written element
    unsigned char * cropTo(unsigned char * aDestination, const Rectangle<int> aZone) const
    {
        int startOffset = (aZone.y()*mDimension.width() + aZone.x());
        for (int line = 0; line != aZone.height(); ++line)
        {
            aDestination = std::copy(get() + (startOffset)*gComponents,
                                     get() + (startOffset + aZone.width())*gComponents,
                                     aDestination);
            startOffset += mDimension.width();
        }
        return aDestination;
    }

    Image crop(const Rectangle<int> aZone) const
    {
        std::unique_ptr<unsigned char[]> target{
            new unsigned char[aZone.mDimension.area() * gComponents]
        };

        cropTo(target.get(), aZone);

        return {target.release(), aZone.mDimension, mSourceComponents};
    }

    Image prepareArray(std::vector<Position2<int>> aPositions,
                       Size2<int> aDimension) const
    {
        std::unique_ptr<unsigned char[]> target{
            new unsigned char[aDimension.area() * gComponents * aPositions.size()]
        };

        unsigned char * destination = target.get();
        for(const auto position : aPositions)
        {
            destination = cropTo(destination, {position, aDimension});
        }

        return {
            target.release(),
            {static_cast<int>(aDimension.width() * aPositions.size()), aDimension.height()},
            mSourceComponents
        };
    }

    std::vector<Image> cutouts(std::vector<Position2<int>> aPositions,
                               Size2<int> aDimension) const
    {
        std::vector<Image> cutouts;
        for(const auto position : aPositions)
        {
            cutouts.push_back(crop({position, aDimension}));
        }
        return cutouts;
    }

    Size2<int> dimension() const
    {
        return mDimension;
    }

    Size2<int> mDimension;
    /// \brief The number of channels in the source image, not in the current data
    int mSourceComponents;

    static constexpr int gComponents{4};

protected:
    Image(unsigned char * aData,
          Size2<int> aDimension,
          int aSourceComponents) :
        ResourceGuard<unsigned char*>(aData, [](unsigned char* data){delete [] data;}),
        mDimension(aDimension),
        mSourceComponents(aSourceComponents)
    {}
};

} // namespace graphics
} // namespace ad
