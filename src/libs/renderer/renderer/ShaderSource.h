#pragma once


#include "commons.h"

#include <filesystem>
#include <functional>
#include <span>
#include <vector>


namespace ad {
namespace graphics {


struct SourceMap
{
    struct Mapping
    {
        std::string mIdentifier;
        std::size_t mLine;
    };

    virtual Mapping getLine(std::size_t aCompiledSourceLine) const = 0;
};


/// \brief Host the shader code string, and provide preprocessing for #include
class ShaderSource
{
    class InclusionSourceMap : public SourceMap
    {
    public:
        using IdentifierId = std::size_t;

        SourceMap::Mapping getLine(std::size_t aCompiledSourceLine) const override;

        IdentifierId registerSource(std::string_view aIdentifier);
        void addLineOrigin(IdentifierId aOrigin, std::size_t aLineNumber);

        static constexpr IdentifierId gInternalSource{0};
        
    private:
        struct OriginalLine
        {
            std::size_t mIdentifierIndex;
            std::size_t mLineNumber;
        };

        std::vector<std::string> mIdentifiers = {"<ShaderSource preprocessor>"};
        // Maps the assembled output lines (vector indices) to corresponding original file.
        std::vector<OriginalLine> mMap;
    };

public:
    /// \brief The lookup function takes the string as found inside the include directive quotes,
    /// and returns (unique pointer to) the opened stream and the identifier for the included content.
    /// \note In the case of including files in nested subfolders, the returned identifier could be the full path
    /// whereas the input (from the include directive) might be a relative path from the current folder.
    using Lookup = std::function<
        std::pair<std::unique_ptr<std::istream>, std::string/*identifier*/>(const std::string &)>;
    
    using Defines = std::span<const MacroDefine>;

    static ShaderSource Preprocess(std::istream & aIn,
                                   const Defines & aMacros,
                                   const std::string & aIdentifier,
                                   const Lookup & aLookup);

    static ShaderSource Preprocess(std::istream && aIn,
                                   const Defines & aMacros,
                                   const std::string & aIdentifier,
                                   const Lookup & aLookup)
    { return Preprocess(aIn, aMacros, aIdentifier, aLookup); }

    // Overloads without Defines
    static ShaderSource Preprocess(std::istream & aIn,
                                   const std::string & aIdentifier,
                                   const Lookup & aLookup)
    { return Preprocess(aIn, Defines{}, aIdentifier, aLookup); }

    static ShaderSource Preprocess(std::istream && aIn,
                                   const std::string & aIdentifier,
                                   const Lookup & aLookup)
    { return Preprocess(aIn, Defines{}, aIdentifier, aLookup); }

    static ShaderSource Preprocess(std::filesystem::path aFile,
                                   const Defines & aMacros = {});

    static ShaderSource Preprocess(const std::string & aString,
                                   const std::string & aIdentifier,
                                   const Defines & aMacros = {});
    
    const std::string & getSource() const
    { return mSource; }

    const InclusionSourceMap & getSourceMap() const
    { return mMap; }

private:
    struct Input
    {
        std::istream & mStream;
        const Defines & mMacros;
        std::string_view mIdentifier;
    };

    struct Assembled
    {
        std::ostringstream mStream;
        InclusionSourceMap mMap;
    };

    static void Preprocess_impl(Input aIn, Assembled & aOut, const Lookup & aLookup);

    ShaderSource(std::string aSource, InclusionSourceMap aMap);

    std::string mSource;
    InclusionSourceMap mMap;
};


} // namespace graphics
} // namespace ad