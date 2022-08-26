// Черепухин Евгений Сергеевич. Итоговый проект 13 спринта. Версия 1.
#pragma once
#include <cassert>
#include <cstdlib>
#include <memory>
#include <new>
#include <utility>

template <typename T>
class RawMemory {
public:
    RawMemory() = default;
    RawMemory(const RawMemory&) = delete;
    RawMemory& operator=(const RawMemory& rhs) = delete;

    explicit RawMemory(size_t capacity)
        : buffer_(Allocate(capacity))
        , capacity_(capacity) {
    }

    RawMemory(RawMemory&& other) noexcept {
        buffer_ = std::move(other.buffer_);
        capacity_ = std::move(other.capacity_);

        other.buffer_ = nullptr;
        other.capacity_ = 0;
    }

    RawMemory& operator=(RawMemory&& rhs) noexcept {
        buffer_ = std::move(rhs.buffer_);
        capacity_ = std::move(rhs.capacity_);

        rhs.buffer_ = nullptr;
        rhs.capacity_ = 0;

        return *this;
    }

    ~RawMemory() {
        Deallocate(buffer_);
    }

    T* operator+(size_t offset) noexcept {
        // Разрешается получать адрес ячейки памяти, следующей за последним элементом массива
        assert(offset <= capacity_);
        return buffer_ + offset;
    }

    const T* operator+(size_t offset) const noexcept {
        return const_cast<RawMemory&>(*this) + offset;
    }

    const T& operator[](size_t index) const noexcept {
        return const_cast<RawMemory&>(*this)[index];
    }

    T& operator[](size_t index) noexcept {
        assert(index < capacity_);
        return buffer_[index];
    }

    void Swap(RawMemory& other) noexcept {
        std::swap(buffer_, other.buffer_);
        std::swap(capacity_, other.capacity_);
    }

    const T* GetAddress() const noexcept {
        return buffer_;
    }

    T* GetAddress() noexcept {
        return buffer_;
    }

    size_t Capacity() const {
        return capacity_;
    }

private:
    // Выделяет сырую память под n элементов и возвращает указатель на неё
    static T* Allocate(size_t n) {
        return n != 0 ? static_cast<T*>(operator new(n * sizeof(T))) : nullptr;
    }

    // Освобождает сырую память, выделенную ранее по адресу buf при помощи Allocate
    static void Deallocate(T* buf) noexcept {
        operator delete(buf);
    }

    T* buffer_ = nullptr;
    size_t capacity_ = 0;
};

template <typename T>
class Vector {
public:
    using iterator = T*;
    using const_iterator = const T*;    

    // Конструктор по умолчанию, не выбрасыват исключений.
    Vector() = default;

    // Конструктор создаёт вектор, заданного размера. Все элементы векторабудут 
    // проинициализированы значением по умолчанию типа Т.
    explicit Vector(size_t size)
        : data_(size)
        , size_(size)
    {
        std::uninitialized_value_construct_n(data_.GetAddress(), size);
    }

    // Копирующий конструктор. Создаёт копию элементов исходного вектора. Имеет вместимость, 
    // равную размеру исходного вектора, то есть выделяет память без запаса.
    Vector(const Vector& other)
        : data_(other.size_)
        , size_(other.size_)
    {
        std::uninitialized_copy_n(other.data_.GetAddress(), other.Size(), this->data_.GetAddress());
    }

    Vector(Vector&& other) noexcept {
        data_ = std::move(other.data_);
        size_ = std::move(other.size_);
        other.size_ = 0;
    }

    Vector& operator=(const Vector& rhs) {
        if (rhs.Size() > data_.Capacity()) {
            Vector rhs_copy(rhs);
            Swap(rhs_copy);
            return *this;
        }

        if (rhs.Size() < Size()) {
            for (size_t i = 0; i < rhs.Size(); ++i) {
                data_[i] = rhs[i];
            }
            std::destroy_n(data_.GetAddress() + rhs.Size(), Size() - rhs.Size());
        }
        else {
            for (size_t i = 0; i < Size(); ++i) {
                data_[i] = rhs[i];
            }
            std::uninitialized_copy_n(rhs.data_.GetAddress() + Size(), rhs.Size() - Size(), data_.GetAddress() + Size());
        }

        size_ = rhs.size_;
        return *this;
    }

    Vector& operator=(Vector&& rhs) noexcept {
        data_ = std::move(rhs.data_);
        size_ = std::move(rhs.size_);
        rhs.size_ = 0;
        return *this;
    }

    // Деструктор. Разрушает содержащиеся в векторе элементы и 
    // освобождает занимаемую ими память.
    ~Vector() {
        std::destroy_n(data_.GetAddress(), size_);
    }
    
    iterator begin() noexcept {
        return data_.GetAddress();
    }

    iterator end() noexcept {
        return data_.GetAddress() + Size();
    }

    const_iterator begin() const noexcept {
        return cbegin();
    }

    const_iterator end() const noexcept {
        return cend();
    }

    const_iterator cbegin() const noexcept {
        return data_.GetAddress();
    }

    const_iterator cend() const noexcept {
        return data_.GetAddress() + Size();
    }

    void Swap(Vector& other) noexcept {
        RawMemory<T> tmp_data = std::move(other.data_);
        size_t tmp_size = std::move(other.size_);

        other.data_ = std::move(data_);
        other.size_ = std::move(size_);

        data_ = std::move(tmp_data);
        size_ = std::move(tmp_size);
    }

    size_t Size() const noexcept {
        return size_;
    }

    size_t Capacity() const noexcept {
        return data_.Capacity();
    }

    const T& operator[](size_t index) const noexcept {
        return const_cast<Vector&>(*this)[index];
    }

    T& operator[](size_t index) noexcept {
        assert(index < size_);
        return data_[index];
    }

    // Резервирует достаточно места, чтобы вместить достаточно места, чтобы вместить
    // количество элементов ,равное capacity. Если новая вместимость не превышает текущую,
    // метод не делает ничего.
    void Reserve(size_t new_capacity) {
        if (new_capacity <= data_.Capacity()) {
            return;
        }
        RawMemory<T> new_data(new_capacity);
        if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
            std::uninitialized_move_n(data_.GetAddress(), size_, new_data.GetAddress());
        }
        else {
            std::uninitialized_copy_n(data_.GetAddress(), size_, new_data.GetAddress());
        }
        std::destroy_n(data_.GetAddress(), size_);
        data_.Swap(new_data);
    }

    void Resize(size_t new_size) {
        if (new_size == Size()) {
            return;
        }
        else if (new_size < Size()) {
            std::destroy_n(data_.GetAddress() + new_size, Size() - new_size);
        }
        else {
            Reserve(new_size);
            std::uninitialized_value_construct_n(data_.GetAddress() + Size(), new_size - Size());
        }
        size_ = new_size;
    }
    
    template<typename Type>
    void PushBack(Type&& value) {
        if (data_.Capacity() > Size()) {
            new (data_.GetAddress() + Size()) T(std::forward<Type>(value));
        }
        else {
            RawMemory<T> new_data(Size() == 0 ? 1 : Size() * 2);
            new (new_data.GetAddress() + Size()) T(std::forward<Type>(value));
            if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
                std::uninitialized_move_n(data_.GetAddress(), Size(), new_data.GetAddress());
            }
            else {
                std::uninitialized_copy_n(data_.GetAddress(), Size(), new_data.GetAddress());
            }
            std::destroy_n(data_.GetAddress(), Size());
            data_.Swap(new_data);
        }
        size_++;
    }

    void PopBack() /* noexcept */ {
        std::destroy_n(data_.GetAddress() + Size() - 1, 1);
        size_--;
    }

    template <typename... Args>
    T& EmplaceBack(Args&&... args) {
        if (data_.Capacity() > Size()) {
            new(data_.GetAddress() + Size())T(std::forward<Args>(args)...);
        }
        else {
            RawMemory<T>new_data(Size() == 0 ? 1 : Size() * 2);
            new(new_data.GetAddress() + Size())T(std::forward<Args>(args)...);
            if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
                std::uninitialized_move_n(data_.GetAddress(), Size(), new_data.GetAddress());
            }
            else {
                std::uninitialized_copy_n(data_.GetAddress(), Size(), new_data.GetAddress());
            }
            std::destroy_n(data_.GetAddress(), Size());
            data_.Swap(new_data);
        }
        return data_[size_++];
    }

    template <typename...Args>
    iterator Emplace(const_iterator pos, Args&&... args) {
        int position = pos - begin();
        if (data_.Capacity() > Size()) {
            try {
                if (pos != end()) {
                    T tmp_obj(std::forward<Args>(args)...);
                    new(end())T(std::forward<T>(data_[Size() - 1]));
                    std::move_backward(begin() + position, end() - 1, end());
                    *(begin() + position) = std::forward<T>(tmp_obj);
                }
                else {
                    new(end())T(std::forward<Args>(args)...);
                }
            }
            catch (...) {
                operator delete(end());
                throw;
            }
        }
        else {
            RawMemory<T> new_data(Size() == 0 ? 1 : Size() * 2);
            new(new_data.GetAddress() + position)T(std::forward<Args>(args)...);
            if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
                std::uninitialized_move_n(data_.GetAddress(), position, new_data.GetAddress());
            }
            else {
                std::uninitialized_copy_n(data_.GetAddress(), position, new_data.GetAddress());
            }
            if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
                std::uninitialized_move_n(data_.GetAddress() + position, Size() - position, new_data.GetAddress() + position + 1);
            }
            else {
                std::uninitialized_copy_n(data_.GetAddress() + position, Size() - position, new_data.GetAddress() + position + 1);
            }
            std::destroy_n(data_.GetAddress(), Size());
            data_.Swap(new_data);
        }
        size_++;
        return begin() + position;
    }

    iterator Erase(const_iterator pos) {
        int position = pos - begin();
        std::move(begin() + position + 1, end(), begin() + position);
        std::destroy_at(end() - 1);
        size_--;
        return(begin() + position);
    }

    iterator Insert(const_iterator pos, const T& value) {
        return Emplace(pos, value);
    }

    iterator Insert(const_iterator pos, T&& value) {
        return Emplace(pos, std::move(value));
    }

private:
    // Вызывает деструкторы n объектов по адресу buf
    static void DestroyN(T* buf, size_t n) noexcept {
        for (size_t i = 0; i != n; ++i) {
            Destroy(buf + i);
        }
    }

    // Создаёт копию объекта elem в сырой памяти по адресу buf
    static void CopyConstruct(T* buf, const T& elem) {
        new (buf)T(elem);
    }

    // Вызывает деструктор объекта по адресу buf
    static void Destroy(T* buf) noexcept {
        buf->~T();
    }

    RawMemory<T> data_;
    size_t size_ = 0;
};