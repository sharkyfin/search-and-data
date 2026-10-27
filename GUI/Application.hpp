#pragma once

#include <future>
#include <string>
#include <string_view>

#include "raylib.h"
#include "benchmark/Benchmark.hpp"
#include "GUI/PeopleModel.hpp"
#include "GUI/UiTheme.hpp"

class Application {
private:
    Font font;
    ArraySequence<Person> people;
    UniquePtr<PeopleModel> model;
    ArraySequence<const Person*> matches;
    ArraySequence<Measurement> measurements;
    int source{0};
    int dictionary{0};
    int field{0};
    int benchmarkSource{0};
    int tab{0};
    int editing{-1};
    int dropdown{-1};
    int recordPage{0};
    int matchPage{0};
    int indexPage{0};
    std::string message;
    bool failed{false};
    char countText[12]{"256"};
    char seedText[12]{"42"};
    char firstText[64]{"Анна"};
    char lastText[64]{"Иванова"};
    char yearText[8]{"2004"};
    char queryText[96]{};
    char benchmarkCount[12]{"10000"};
    char benchmarkSeed[12]{"42"};
    std::future<ArraySequence<Measurement>> comparison;

    template <class Action>
    void Try(Action action) {
        try {
            message.clear();
            failed = false;
            action();
        } catch (const std::exception& exception) {
            message = exception.what();
            failed = true;
        }
    }

    static int ReadNumber(std::string_view text, int minimum, int maximum);
    void Rebuild(ArraySequence<Person> records, int nextSource, int nextDictionary, int nextField);
    void Search();
    void StartComparison();
    void PollComparison();
    void SaveCsv();
    void Label(const std::string& text, float x, float y, float size = 17, Color color = UiTheme::Ink) const;
    void Clipped(const std::string& text, float x, float y, float width,
                 float size = 17, Color color = UiTheme::Ink) const;
    void Panel(Rectangle bounds) const;
    bool Button(Rectangle bounds, const char* text, bool enabled = true);
    void Input(const char* label, float x, float y, float width, char* buffer, int capacity, int id);
    void TextBox(Rectangle bounds, char* buffer, int capacity, int id);
    void Pager(int& page, int count, int pageSize, float x, float y, float width);
    void DrawChoices(bool expanded = false);
    void DrawNavigation();
    void DrawIndexControls();
    void PersonHeaders(float y) const;
    void PersonRow(const Person& person, float y) const;
    void DrawRecords();
    void DrawSearch();
    template <class Key> void DrawEntries(const Index<Person, Key>& index);
    void DrawIndexEntries();
    void DrawComparison();
    void DrawComparisonField(const char* key, const char* title, float y);

public:
    explicit Application(Font font);
    void PreparePreview(bool benchmark);
    bool IsBusy() const { return comparison.valid(); }
    void Draw();
};
