#include <gtest/gtest.h>

#include "renderer/raylib_helpers/I18n.hpp"

class I18nTest : public ::testing::Test {
    protected:
    void SetUp() override { I18n::setLanguage(I18n::Language::EN); }
};

TEST_F(I18nTest, DefaultLanguageIsEN)
{
    EXPECT_EQ(I18n::getLanguage(), I18n::Language::EN);
}

TEST_F(I18nTest, ENStringsNotEmpty)
{
    I18n::setLanguage(I18n::Language::EN);
    for (int k = 0; k < static_cast<int>(I18n::Key::_COUNT); k++)
        EXPECT_NE(I18n::get(static_cast<I18n::Key>(k)), nullptr) << "key " << k << " is null";
}

TEST_F(I18nTest, FRStringsNotEmpty)
{
    I18n::setLanguage(I18n::Language::FR);
    for (int k = 0; k < static_cast<int>(I18n::Key::_COUNT); k++)
        EXPECT_NE(I18n::get(static_cast<I18n::Key>(k)), nullptr) << "key " << k << " is null";
}

TEST_F(I18nTest, LanguageSwitchChangesStrings)
{
    I18n::setLanguage(I18n::Language::EN);
    std::string en = I18n::get(I18n::Key::LABEL_TILE);

    I18n::setLanguage(I18n::Language::FR);
    std::string fr = I18n::get(I18n::Key::LABEL_TILE);

    EXPECT_NE(en, fr);
}

TEST_F(I18nTest, ResourceNameIndexMatchesResourceOrder)
{
    I18n::setLanguage(I18n::Language::EN);
    EXPECT_STREQ(I18n::resourceName(0), "Food");
    EXPECT_STREQ(I18n::resourceName(6), "Thystame");
}

TEST_F(I18nTest, FRResourceNameIndexMatchesResourceOrder)
{
    I18n::setLanguage(I18n::Language::FR);
    EXPECT_STREQ(I18n::resourceName(0), "Nourriture");
    EXPECT_STREQ(I18n::resourceName(6), "Thystame");
}
