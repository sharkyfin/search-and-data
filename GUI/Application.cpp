#include "GUI/Application.hpp"

#include <algorithm>
#include <charconv>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <limits>

#include "raygui.h"
#include "benchmark/CsvExporter.hpp"

static constexpr const char* DictionaryNames[]{"SortedDictionary", "TreeDictionary", "HashTable"};
static std::string KeyText(const std::string& key) { return key; }
static std::string KeyText(int key) { return std::to_string(key); }
static std::string TimeText(double milliseconds) {
    char text[40];
    std::snprintf(text, sizeof(text), "%.3f", milliseconds);
    return text;
}

int Application::ReadNumber(std::string_view text, int minimum, int maximum) {
    int value = 0;
    const auto parsed = std::from_chars(text.data(), text.data() + text.size(), value);
    if (parsed.ec != std::errc() || parsed.ptr != text.data() + text.size()
        || value < minimum || value > maximum) {
        throw std::invalid_argument("Введите целое число от " + std::to_string(minimum)
                                    + " до " + std::to_string(maximum) + ".");
    }
    return value;
}

void Application::Rebuild(ArraySequence<Person> records, int nextSource, int nextDictionary, int nextField) {
    auto next = MakeUnique<PeopleModel>(records, nextSource, nextDictionary, nextField);
    matches.Clear();
    model = Move(next);
    people = Move(records);
    source = nextSource;
    dictionary = nextDictionary;
    field = nextField;
    recordPage = matchPage = indexPage = 0;
    std::string query = field == 0 ? "Ivanov" : field == 1 ? "2004" : "1";
    if (!model->GetRecords().IsEmpty()) {
        const auto& first = *model->GetRecords().GetFirst();
        query = field == 0 ? first.lastName : std::to_string(field == 1 ? first.birthYear : first.id);
    }
    std::snprintf(queryText, sizeof(queryText), "%s", query.c_str());
    Search();
}

void Application::Search() {
    if (field == 0) matches = model->GetNameIndex()->FindAll(queryText);
    else matches = model->GetNumberIndex()->FindAll(ReadNumber(queryText,
        field == 1 ? 1000 : 1, field == 1 ? 2100 : std::numeric_limits<int>::max()));
    matchPage = 0;
}

void Application::StartComparison() {
    BenchmarkOptions options;
    options.records = ReadNumber(benchmarkCount, 1, 1000000);
    options.seed = static_cast<unsigned int>(ReadNumber(benchmarkSeed, 0, std::numeric_limits<int>::max()));
    const int choice = benchmarkSource + 1;
    measurements.Clear();
    comparison = std::async(std::launch::async, [choice, options] { return RunBenchmarks(choice, options); });
}

void Application::PollComparison() {
    if (comparison.valid() && comparison.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) {
        Try([&] {
            measurements = comparison.get();
            message = "Замеры завершены. CSV содержит медианы и разброс повторений.";
        });
    }
}

void Application::SaveCsv() {
    std::filesystem::path path = "benchmark-gui.csv";
    for (int number = 1; std::filesystem::exists(path); ++number) {
        path = "benchmark-gui-" + std::to_string(number) + ".csv";
    }
    ExportMeasurements(measurements, path.string());
    message = "Сохранено: " + std::filesystem::absolute(path).string();
}

void Application::Label(const std::string& text, float x, float y, float size, Color color) const {
    DrawTextEx(font, text.c_str(), Vector2{x, y}, size, 0.3f, color);
}

void Application::Clipped(const std::string& text, float x, float y, float width, float size, Color color) const {
    BeginScissorMode(static_cast<int>(x), static_cast<int>(y), static_cast<int>(width), 25);
    Label(text, x, y, size, color);
    EndScissorMode();
}

void Application::Panel(Rectangle bounds) const {
    DrawRectangleRec(bounds, WHITE);
    DrawRectangleLinesEx(bounds, 1, UiTheme::Border);
}

bool Application::Button(Rectangle bounds, const char* text, bool enabled) {
    const int state = GuiGetState();
    if (!enabled) GuiDisable();
    const bool pressed = GuiButton(bounds, text) != 0;
    GuiSetState(state);
    return pressed;
}

void Application::TextBox(Rectangle bounds, char* buffer, int capacity, int id) {
    if (GuiTextBox(bounds, buffer, capacity, editing == id)) editing = editing == id ? -1 : id;
}

void Application::Input(const char* label, float x, float y, float width, char* buffer, int capacity, int id) {
    Label(label, x, y, 15, UiTheme::Muted);
    TextBox(Rectangle{x, y + 25, width, 34}, buffer, capacity, id);
}

void Application::Pager(int& page, int count, int pageSize, float x, float y, float width) {
    const int pages = std::max(1, (count + pageSize - 1) / pageSize);
    page = std::clamp(page, 0, pages - 1);
    Label("Всего: " + std::to_string(count), x, y + 6, 14, UiTheme::Muted);
    if (Button(Rectangle{x + width - 178, y, 32, 28}, "<", page > 0)) --page;
    Label(std::to_string(page + 1) + " / " + std::to_string(pages), x + width - 132, y + 6, 14);
    if (Button(Rectangle{x + width - 32, y, 32, 28}, ">", page + 1 < pages)) ++page;
}

void Application::DrawChoices(bool expanded) {
    const char* options[]{
        "ArraySequence;BinaryTreeSet;SortedArraySequence;SortedDictionary;TreeDictionary;HashTable",
        "SortedDictionary;TreeDictionary;HashTable", "Фамилия;Год рождения;ID"
    };
    const char* labels[]{"Источник записей", "Словарь индекса", "Ключ индекса"};
    const int values[]{source, dictionary, field, benchmarkSource};
    for (int i = tab == 0 ? 0 : 3; i < (tab == 0 ? 3 : 4); ++i) {
        if (expanded && dropdown != i) continue;
        const Rectangle bounds = i == 3 ? Rectangle{44, 185, 310, 34}
                                       : Rectangle{44, 142 + static_cast<float>(i) * 78, 236, 34};
        if (!expanded) Label(i == 3 ? labels[0] : labels[i], bounds.x, bounds.y - 26, 15, UiTheme::Muted);
        int selected = values[i];
        if (!GuiDropdownBox(bounds, options[i == 3 ? 0 : i], &selected, expanded)) continue;
        editing = -1;
        dropdown = expanded ? -1 : i;
        if (selected == values[i]) continue;
        Try([&] {
            if (i == 3) {
                benchmarkSource = selected;
                measurements.Clear();
            } else {
                Rebuild(people, i == 0 ? selected : source,
                        i == 1 ? selected : dictionary, i == 2 ? selected : field);
            }
        });
    }
}

void Application::DrawNavigation() {
    Label("Индексация", 24, 24, 30);
    int selected = tab;
    GuiToggleGroup(Rectangle{802, 22, 176, 38}, "Записи и индекс;Бенчмарк", &selected);
    if (selected != tab) {
        tab = selected;
        editing = -1;
        dropdown = -1;
        message.clear();
    }
    DrawLine(24, 80, 1156, 80, UiTheme::Border);
}

void Application::DrawIndexControls() {
    Panel(Rectangle{24, 100, 276, 714});
    DrawChoices();
    DrawLine(44, 348, 280, 348, UiTheme::Border);
    Input("Записей", 44, 366, 126, countText, sizeof(countText), 0);
    Input("Seed", 186, 366, 94, seedText, sizeof(seedText), 1);
    if (Button(Rectangle{44, 440, 236, 36}, "Сгенерировать")) {
        Try([&] {
            Rebuild(GeneratePeople(ReadNumber(countText, 0, 10000),
                static_cast<unsigned int>(ReadNumber(seedText, 0, std::numeric_limits<int>::max()))),
                source, dictionary, field);
        });
    }
    DrawLine(44, 492, 280, 492, UiTheme::Border);
    Input("Имя", 44, 508, 236, firstText, sizeof(firstText), 2);
    Input("Фамилия", 44, 580, 236, lastText, sizeof(lastText), 3);
    Input("Год рождения", 44, 652, 112, yearText, sizeof(yearText), 4);
    if (Button(Rectangle{168, 677, 112, 34}, "Добавить")) {
        Try([&] {
            if (std::string_view(firstText).find_first_not_of(" \t\r\n") == std::string_view::npos
                || std::string_view(lastText).find_first_not_of(" \t\r\n") == std::string_view::npos) {
                throw std::invalid_argument("Введите имя и фамилию.");
            }
            if (people.GetCount() >= 10000) throw std::invalid_argument("Максимум 10000 записей.");
            auto next = people;
            next.Append(Person{people.GetCount() + 1, firstText, lastText, ReadNumber(yearText, 1000, 2100)});
            Rebuild(Move(next), source, dictionary, field);
        });
    }
    if (Button(Rectangle{44, 756, 236, 34}, "Очистить записи", !people.IsEmpty())) {
        Try([&] { Rebuild(ArraySequence<Person>(), source, dictionary, field); });
    }
}

void Application::PersonHeaders(float y) const {
    Label("ID", 344, y, 13, UiTheme::Muted);
    Label("Имя", 492, y, 13, UiTheme::Muted);
    Label("Фамилия", 706, y, 13, UiTheme::Muted);
    Label("Год", 1060, y, 13, UiTheme::Muted);
}

void Application::PersonRow(const Person& person, float y) const {
    Label(std::to_string(person.id), 344, y, 16);
    Clipped(person.firstName, 492, y, 190, 16);
    Clipped(person.lastName, 706, y, 326, 16);
    Label(std::to_string(person.birthYear), 1060, y, 16);
}

void Application::DrawRecords() {
    Panel(Rectangle{324, 100, 832, 298});
    Label("Записи исходного контейнера", 344, 116, 19);
    PersonHeaders(150);
    DrawLine(344, 174, 1136, 174, UiTheme::Border);
    const auto& records = model->GetRecords();
    for (int row = 0; row < 6; ++row) {
        const int position = recordPage * 6 + row;
        if (position >= records.GetCount()) break;
        PersonRow(*records.Get(position), 184 + static_cast<float>(row) * 26);
    }
    if (records.IsEmpty()) Label("Нет записей.", 344, 216, 17, UiTheme::Muted);
    Pager(recordPage, records.GetCount(), 6, 344, 356, 792);
}

void Application::DrawSearch() {
    Panel(Rectangle{324, 414, 832, 200});
    Label("Поиск", 344, 430, 19);
    const bool enter = editing == 5 && IsKeyPressed(KEY_ENTER);
    TextBox(Rectangle{344, 460, 638, 34}, queryText, sizeof(queryText), 5);
    if (Button(Rectangle{996, 460, 140, 34}, "Найти") || enter) Try([&] { Search(); });
    PersonHeaders(505);
    for (int row = 0; row < 2; ++row) {
        const int position = matchPage * 2 + row;
        if (position >= matches.GetCount()) break;
        PersonRow(*matches.Get(position), 533 + static_cast<float>(row) * 24);
    }
    Pager(matchPage, matches.GetCount(), 2, 344, 582, 792);
}

template <class Key>
void Application::DrawEntries(const Index<Person, Key>& index) {
    auto entries = index.GetEntriesEnumerator();
    for (int i = 0; i < indexPage * 3; ++i) if (!entries->MoveNext()) break;
    for (int row = 0; row < 3 && entries->MoveNext(); ++row) {
        const auto& entry = entries->CurrentRef();
        std::string ids;
        for (int i = 0; i < entry.value.GetCount() && i < 8; ++i) {
            if (i) ids += ", ";
            ids += std::to_string(entry.value.Get(i)->id);
        }
        if (entry.value.GetCount() > 8) ids += ", ...";
        const float y = 701 + static_cast<float>(row) * 24;
        Clipped(KeyText(entry.key), 344, y, 238, 16);
        Clipped(ids, 602, y, 432, 16);
        Label(std::to_string(entry.value.GetCount()), 1060, y, 16);
    }
}

void Application::DrawIndexEntries() {
    Panel(Rectangle{324, 630, 832, 184});
    Label("Индекс", 344, 646, 19);
    Label(DictionaryNames[dictionary], 452, 649, 15, UiTheme::Muted);
    Label("Ключ", 344, 678, 13, UiTheme::Muted);
    Label("ID записей", 602, 678, 13, UiTheme::Muted);
    Label("Найдено", 1060, 678, 13, UiTheme::Muted);
    if (model->GetNameIndex()) DrawEntries(*model->GetNameIndex());
    else DrawEntries(*model->GetNumberIndex());
    if (model->GetKeyCount() == 0) Label("Индекс пуст.", 344, 718, 17, UiTheme::Muted);
    Pager(indexPage, model->GetKeyCount(), 3, 344, 780, 792);
}

void Application::DrawComparisonField(const char* key, const char* title, float y) {
    Label(title, 44, y, 20);
    Label("Индекс", 44, y + 29, 13, UiTheme::Muted);
    Label("Построение, мс", 400, y + 29, 13, UiTheme::Muted);
    Label("100 000 запросов, мс", 620, y + 29, 13, UiTheme::Muted);
    Label("Всего, мс", 882, y + 29, 13, UiTheme::Muted);
    const Measurement* fastestBuild = nullptr;
    const Measurement* fastestSearch = nullptr;
    int position = 0;
    for (int i = 0; i < measurements.GetCount(); ++i) {
        const auto& row = measurements.Get(i);
        if (row.field != key) continue;
        const float rowY = y + 54 + static_cast<float>(position++) * 42;
        Label(row.algorithm, 44, rowY, 16);
        if (row.status != "ok") {
            Label(row.status == "source_timeout" ? "Превышен лимит заполнения источника"
                : row.status == "build_timeout" ? "Превышен лимит построения" : "Превышен лимит поиска",
                400, rowY, 15, UiTheme::Error);
            continue;
        }
        Label(TimeText(row.build.median), 400, rowY, 16);
        Label(TimeText(row.search.median), 620, rowY, 16);
        Label(TimeText(row.total.median), 882, rowY, 16);
        Label("Поиск: " + TimeText(row.search.minimum) + ".." + TimeText(row.search.maximum)
            + " мс; ключей: " + std::to_string(row.keys), 400, rowY + 20, 12, UiTheme::Muted);
        if (!fastestBuild || row.build.median < fastestBuild->build.median) fastestBuild = &row;
        if (!fastestSearch || row.search.median < fastestSearch->search.median) fastestSearch = &row;
    }
    if (fastestBuild) Label("Построение: " + fastestBuild->algorithm + ". Поиск: " + fastestSearch->algorithm,
                            44, y + 188, 15, UiTheme::Accent);
}

void Application::DrawComparison() {
    Panel(Rectangle{24, 100, 1132, 714});
    Label("Бенчмарк индексов", 44, 118, 22);
    const bool running = comparison.valid();
    if (running) GuiDisable();
    DrawChoices();
    Input("Записей (1..1000000)", 380, 160, 142, benchmarkCount, sizeof(benchmarkCount), 6);
    Input("Seed", 548, 160, 140, benchmarkSeed, sizeof(benchmarkSeed), 7);
    if (Button(Rectangle{724, 185, 190, 34}, "Сравнить")) Try([&] { StartComparison(); });
    GuiEnable();
    if (Button(Rectangle{928, 185, 208, 34}, "Сохранить CSV", !running && !measurements.IsEmpty())) {
        Try([&] { SaveCsv(); });
    }
    Label("Отдельный тест на сгенерированных записях. Один прогрев, пять повторений, 25% промахов.",
          44, 237, 15, UiTheme::Muted);
    if (measurements.IsEmpty()) {
        Label(running ? "Выполняются замеры. Окно остаётся доступным." : "Выберите источник и запустите сравнение.",
              44, 300, 18, UiTheme::Muted);
        return;
    }
    DrawComparisonField("birthYear", "Год рождения", 280);
    DrawComparisonField("lastName", "Фамилия", 496);
    const auto& result = measurements.GetFirst();
    Label("Результат: " + result.source + ", " + std::to_string(result.records) + " записей, seed "
        + std::to_string(result.seed) + ". Заполнение: " + TimeText(result.sourceMilliseconds) + " мс.",
        44, 732, 14, UiTheme::Muted);
    Label("Всего = заполнение + построение + запросы. Лимит каждой фазы: 5 секунд.", 44, 760, 14, UiTheme::Muted);
}

Application::Application(Font font) : font(font) {
    Rebuild(GeneratePeople(256, 42), source, dictionary, field);
}

void Application::PreparePreview(bool benchmark) {
    if (!benchmark) return;
    tab = 1;
    std::snprintf(benchmarkCount, sizeof(benchmarkCount), "%d", 256);
    StartComparison();
}

void Application::Draw() {
    PollComparison();
    const bool menuOpen = dropdown >= 0;
    if (menuOpen) GuiLock();
    ClearBackground(UiTheme::Background);
    DrawNavigation();
    if (tab == 0) {
        DrawIndexControls();
        DrawRecords();
        DrawSearch();
        DrawIndexEntries();
    } else DrawComparison();
    GuiUnlock();
    if (menuOpen) DrawChoices(true);
    Clipped(message, 24, 832, 1132, 15, failed ? UiTheme::Error : UiTheme::Accent);
}
