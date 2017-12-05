#define THEORIDE_STRINGIZE(...)       THEORIDE_STRINGIZE_I(__VA_ARGS__)
#define THEORIDE_STRINGIZE_I(...)     #__VA_ARGS__ ""

inline std::string to_string(THEORIDE_ENUM iEnum, unsigned=0)
{
    #define THEORIDE_ELEMENT(dSymbol)\
        case THEORIDE_ENUM::dSymbol: \
            return "\"" THEORIDE_STRINGIZE(THEORIDE_ENUM) "::" #dSymbol "\"";

    switch(iEnum)
    {
        THEORIDE_ENUM_LIST()
    }
    #undef THEORIDE_ELEMENT
    return "\"" THEORIDE_STRINGIZE(THEORIDE_ENUM) "::<unknown>\"";
}

inline std::ostream& operator<<(std::ostream& os, THEORIDE_ENUM iEnum)
{
    os << to_string(iEnum);
    return os;
}

#undef THEORIDE_STRINGIZE
#undef THEORIDE_STRINGIZE_I
