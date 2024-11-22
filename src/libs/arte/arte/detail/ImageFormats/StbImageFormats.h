#pragma once


#include "../../Image.h"

#include "../3rdparty/stb_image_include.h"
#include "../3rdparty/stb_image_write_include.h"

#include <cassert>


namespace ad {
namespace arte {
namespace detail {


constexpr int gJpegQuality = 90;


template <class T_pixel>
struct stbi_traits;


template <>
struct stbi_traits<math::sdr::Grayscale>
{
    static constexpr int channels = STBI_grey;
    static constexpr auto loadFromCallbacks = &stbi_load_from_callbacks;
};


template <>
struct stbi_traits<math::sdr::Rgb>
{
    static constexpr int channels = STBI_rgb;
    static constexpr auto loadFromCallbacks = &stbi_load_from_callbacks;
};


template <>
struct stbi_traits<math::sdr::Rgba>
{
    static constexpr int channels = STBI_rgb_alpha;
    static constexpr auto loadFromCallbacks = &stbi_load_from_callbacks;
};


template <class T_number>
struct stbi_traits<math::hdr::Rgb<T_number>>
{
    static constexpr int channels = STBI_rgb;
    // IMPORTANT: load**f**
    static constexpr auto loadFromCallbacks = &stbi_loadf_from_callbacks;
};


template <class T_number>
struct stbi_traits<math::hdr::Rgba<T_number>>
{
    static constexpr int channels = STBI_rgb_alpha;
    // IMPORTANT: load**f**
    static constexpr auto loadFromCallbacks = &stbi_loadf_from_callbacks;
};


struct StbImageFormats
{
    inline static const stbi_io_callbacks streamCallbacks{
        // Read
        [](void *user,char *data,int size) -> int
        {
            std::istream * input = reinterpret_cast<std::istream *>(user);
            input->read(data, size);
            return static_cast<int>(input->gcount());
        },
        // Skip
        [](void *user,int n)
        {
            std::istream * input = reinterpret_cast<std::istream *>(user);
            input->seekg(n, std::ios_base::cur);
        },
        // Eof
        [](void *user) -> int
        {
            std::istream * input = reinterpret_cast<std::istream *>(user);
            return input->eof() ? 1 : 0;
        }
    };


    template <class T_pixel>
    static Image<T_pixel> Read(std::istream & aIn, ImageOrientation aOrientation)
    {
        stbi_set_flip_vertically_on_load(aOrientation == ImageOrientation::InvertVerticalAxis);

        math::Size<2, int> dimension = math::Size<2, int>::Zero();
        int channelsInFile;

        // stbi_traits will redirect to either `load` or `loadf` based on pixel type.
        unsigned char * data = (unsigned char *)stbi_traits<T_pixel>::loadFromCallbacks(
            &streamCallbacks,
            &aIn,
            &dimension.width(),
            &dimension.height(),
            &channelsInFile,
            stbi_traits<T_pixel>::channels);

        // Note: there are probably valid situtations where those will not match
        // handle the cases when they show up.
        assert(channelsInFile == stbi_traits<T_pixel>::channels);

        // HDR CASE
        // IMPORTANT: even though it returns a float*, stbi_loadf internally allocates
        // the returned data buffer as a char* via our provided STBI_MALLOC.
        // So the default Deleter is right.
        // TODO Ad 2022/06/08: I am nonetheless not sure whether it is safe to
        // then treat the returned array as an array of T_pixel (math::hdr::Rgb_f).
        // overview of the issue: https://stackoverflow.com/a/70157161/1027706
        return Image<T_pixel>{dimension, std::unique_ptr<unsigned char []>{data}};
    };
    

    static void WriteCallback(void * aContext, void * aData, int aSize)
    {
        std::ostream * aOut = reinterpret_cast<std::ostream *>(aContext);
        if(!aOut->write(static_cast<char *>(aData), aSize))
        {
            throw std::runtime_error{"Could not write to output stream."};
        }
    }


    template <class T_pixel>
    static void Write(std::ostream & aOut,
                      const Image<T_pixel> & aImage,
                      ImageFormat aFormat,
                      ImageOrientation aOrientation)
    {
        stbi_flip_vertically_on_write(aOrientation == ImageOrientation::InvertVerticalAxis);

        switch(aFormat)
        {
        case ImageFormat::Bmp:
            stbi_write_bmp_to_func(&WriteCallback, &aOut,
                                   aImage.width(), aImage.height(),
                                   stbi_traits<T_pixel>::channels,
                                   aImage.data());
        case ImageFormat::Jpg:
            stbi_write_jpg_to_func(&WriteCallback, &aOut,
                                   aImage.width(), aImage.height(),
                                   stbi_traits<T_pixel>::channels,
                                   aImage.data(),
                                   gJpegQuality);
        case ImageFormat::Png:
            stbi_write_png_to_func(&WriteCallback, &aOut,
                                   aImage.width(), aImage.height(),
                                   stbi_traits<T_pixel>::channels,
                                   aImage.data(),
                                   (int)aImage.size_bytes_line());
            break;
        default:
            throw std::runtime_error{"STB does not write format: " + to_string(aFormat)};
        }
    };

    static void WriteHdr(std::ostream & aOut,
                         const Image<math::hdr::Rgb_f> & aImage,
                         ImageOrientation aOrientation)
    {
        stbi_flip_vertically_on_write(aOrientation == ImageOrientation::InvertVerticalAxis);

        stbi_write_hdr_to_func(&WriteCallback, &aOut,
                                aImage.width(), aImage.height(),
                                stbi_traits<math::hdr::Rgb_f>::channels,
                                aImage.data()->data());
    }
};


} // namespace detail
} // namespace arte
} // namespace ad
