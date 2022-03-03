#ifndef VECTOR_H_
#define VECTOR_H_

#include <fstream>
#include <functional>
#include <memory>
#include <type_traits>

namespace lab_07 {

std::size_t get_deg_2(std::size_t);

template <typename T, typename Alloc = std::allocator<T>>
class vector {
    static_assert(std::is_nothrow_move_constructible_v<T>);
    static_assert(std::is_nothrow_move_assignable_v<T>);
    static_assert(std::is_nothrow_destructible_v<T>);

    class wrapper {
        std::size_t size, capacity;
        T *data{nullptr};

    public:
        wrapper(const wrapper &) = delete;
        wrapper &operator=(const wrapper &) = delete;
        wrapper &operator=(wrapper &&) = delete;

        wrapper(wrapper &&other) noexcept
            : size(std::exchange(other.size, 0)),
              capacity(std::exchange(other.capacity, 0)),
              data(std::exchange(other.data, nullptr)) {
        }

        wrapper(std::size_t n, std::size_t m) : size(n), capacity(m) {
            if (capacity != 0) {
                data = Alloc().allocate(capacity);
            }
        }

        ~wrapper() {
            for (std::size_t i = 0; i < size; ++i) {
                data[i].~T();
            }
            if (capacity != 0) {
                Alloc().deallocate(data, capacity);
            }
        }

        friend class vector;

        friend void swap(wrapper &first, wrapper &second) noexcept {
            std::swap(first.size, second.size);
            std::swap(first.capacity, second.capacity);
            std::swap(first.data, second.data);
        }
    } wrap;

public:
    vector() noexcept : wrap(0, 0) {
    }
    explicit vector(std::size_t n) : wrap(n, get_deg_2(n)) {
        for (std::size_t i = 0; i < n; ++i) {
            if (wrap.data != nullptr) {
                new (wrap.data + i) T();
            }
        }
    }

private:
    void common_vector_constructors_with_copy(
        T *pointer_at_elem,
        const std::function<void(T *, std::size_t)> &create_cell) {
        for (std::size_t i = 0; i < wrap.size; ++i) {
            try {
                if (wrap.data != nullptr) {
                    create_cell(pointer_at_elem, i);
                }
            } catch (...) {
                for (std::size_t j = 0; j < i; ++j) {
                    wrap.data[j].~T();
                }
                wrap.size = 0;  //Теперь в деструкторе не уничтожаем
                                //несуществующие элементы
                throw;
            }
        }
    }

public:
    vector(std::size_t n, T t) : wrap(n, get_deg_2(n)) {
        common_vector_constructors_with_copy(
            &t, [&](T *t, std::size_t i) { new (wrap.data + i) T(*t); });
    }

    vector(const vector &other) : wrap(other.size(), get_deg_2(other.size())) {
        common_vector_constructors_with_copy(
            other.wrap.data, [&](T *other_data, std::size_t i) {
                new (wrap.data + i) T(other_data[i]);
            });
    }

    vector &operator=(const vector &other) {
        if (this == &other) {
            return *this;
        }
        vector c(other);
        swap(*this, c);
        return *this;
    }

    vector(vector &&other) noexcept : wrap(std::move(other.wrap)) {
    }

    vector &operator=(vector &&other) noexcept {
        if (this == &other) {
            return *this;
        }
        std::size_t old_capacity = wrap.capacity;
        vector new_vec(std::move(other));
        swap(*this, new_vec);
        other.reserve(old_capacity);
        return *this;
    }

    [[nodiscard]] bool empty() const noexcept {
        return wrap.size == 0;
    }

    [[nodiscard]] std::size_t size() const noexcept {
        return wrap.size;
    }

    [[nodiscard]] std::size_t capacity() const noexcept {
        return wrap.capacity;
    }

private:
    void check_size(std::size_t i) const {
        if (i >= wrap.size) {
            throw std::out_of_range("out_of_range");
        }
    }

public:
    T &at(std::size_t i) & {
        check_size(i);
        return wrap.data[i];
    }

    T &&at(std::size_t i) && {
        check_size(i);
        return std::move(wrap.data[i]);
    }

    const T &at(std::size_t i) const & {
        check_size(i);
        return wrap.data[i];
    }

    T &operator[](std::size_t i) &noexcept {
        return wrap.data[i];
    }

    T &&operator[](std::size_t i) &&noexcept {
        return std::move(wrap.data[i]);
    }

    const T &operator[](std::size_t i) const & {
        return wrap.data[i];
    }

private:
    void common_push_back(const std::function<void(T *)> &push) {
        if (wrap.size + 1 > wrap.capacity) {
            wrapper new_wrap(0, wrap.capacity == 0 ? 1 : 2 * wrap.capacity);
            push(new_wrap.data);
            new_wrap.size = wrap.size + 1;
            for (std::size_t i = 0; i < wrap.size; ++i) {
                new (new_wrap.data + i) T(std::move(wrap.data[i]));
            }
            swap(wrap, new_wrap);
        } else {
            push(wrap.data);
            ++wrap.size;
        }
    }

public:
    void push_back(T &&t) {
        common_push_back(
            [&](T *data) { new (data + wrap.size) T(std::move(t)); });
    }

    void push_back(const T &t) {
        common_push_back([&](T *data) { new (data + wrap.size) T(t); });
    }

    void pop_back() {
        if (wrap.size == 0) {
            throw std::out_of_range("vector is empty");
        }
        --wrap.size;
        wrap.data[wrap.size].~T();
    }

    void reserve(std::size_t k) noexcept {
        if (k <= wrap.capacity) {
            return;
        }
        std::size_t new_capacity = get_deg_2(k);
        T *old_data = wrap.data;
        wrap.data = Alloc().allocate(new_capacity);
        for (std::size_t i = 0; i < wrap.size; ++i) {
            new (wrap.data + i) T(std::move(old_data[i]));
            old_data[i].~T();
        }
        if (wrap.capacity != 0) {
            Alloc().deallocate(old_data, wrap.capacity);
        }
        wrap.capacity = new_capacity;
    }

    void clear() noexcept {
        for (std::size_t i = 0; i < wrap.size; ++i) {
            wrap.data[i].~T();
        }
        wrap.size = 0;
    }

private:
    void common_resize(
        std::size_t n,
        [[maybe_unused]] const T &t,
        const std::function<void(T *, std::size_t)> &create_cell) {
        if (n > wrap.size) {
            vector new_vec;
            new_vec.reserve(std::max(wrap.capacity, n));
            for (std::size_t i = wrap.size; i < n; ++i) {
                try {
                    create_cell(new_vec.wrap.data, i);
                } catch (...) {
                    for (std::size_t j = wrap.size; j < i; ++j) {
                        new_vec.wrap.data[j].~T();
                    }
                    throw;
                }
            }
            new_vec.wrap.size = n;
            for (std::size_t i = 0; i < wrap.size; ++i) {
                new (new_vec.wrap.data + i) T(std::move(wrap.data[i]));
            }
            swap(*this, new_vec);
        } else if (n < wrap.size) {
            std::size_t iterations = wrap.size - n;
            for (std::size_t i = 0; i < iterations; ++i) {
                pop_back();
            }
        }
    }

public:
    void resize(std::size_t n) {
        common_resize(n, T(),
                      [&](T *data, std::size_t i) { new (data + i) T(); });
    }

    void resize(std::size_t n, const T &t) {
        common_resize(n, t,
                      [&](T *data, std::size_t i) { new (data + i) T(t); });
    }

    ~vector() = default;

    friend void swap(vector<T, Alloc> &first,
                     vector<T, Alloc> &second) noexcept {
        swap(first.wrap, second.wrap);
    }
};

}  // namespace lab_07

#endif  // VECTOR_H_
