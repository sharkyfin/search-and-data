#pragma once

template <class T>
class IEnumerator {
public:
    virtual ~IEnumerator() = default;
    virtual bool MoveNext() = 0;
    // Ссылка на элемент источника, а не на временную копию в перечислителе.
    // Чтение и продвижение перечислителя не меняют адрес элемента.
    virtual const T& CurrentRef() const = 0;
};
