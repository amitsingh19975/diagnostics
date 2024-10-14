#ifndef DARK_DIAGNOSTICS_FUNCTION_REF_HPP
#define DARK_DIAGNOSTICS_FUNCTION_REF_HPP

#include <type_traits>
#include <utility>

namespace dark::core {

    template<typename Ret, typename ...Params>
    class FunctionRef;

    namespace detail {
        template<typename T>
        struct is_function_ref_helper: std::false_type {};

        template<typename Ret, typename ...Params>
        struct is_function_ref_helper<Ret(Params...)>: std:: true_type {};

        template<typename T>
        concept is_function_ref = is_function_ref_helper<std::remove_cvref_t<T>>::value;
    } // namespace detail

    template<typename Ret, typename ...Params>
    class FunctionRef<Ret(Params...)> {
        Ret (*callback)(std::intptr_t callable, Params ...params) = nullptr;
        std::intptr_t callable;

        template<typename Callable>
        static Ret callback_fn(std::intptr_t callable, Params ...params) {
            return (*reinterpret_cast<Callable*>(callable))(
                std::forward<Params>(params)...);
        }

    public:
        FunctionRef() = default;
        FunctionRef(std::nullptr_t) {}

        template <typename Callable>
            requires (!detail::is_function_ref<Callable> && (std::is_invocable_r_v<Ret, Callable, Params...>))
        FunctionRef(Callable &&callable)
            : callback(callback_fn<std::remove_reference_t<Callable>>),
            callable(reinterpret_cast<std::intptr_t>(&callable)) {}

        Ret operator()(Params ...params) const {
            return callback(callable, std::forward<Params>(params)...);
        }

        explicit operator bool() const { return callback; }
    };

} // namespace dark::core

#endif // DARK_DIAGNOSTICS_FUNCTION_REF_HPP
