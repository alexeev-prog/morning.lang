#include <string>

#include "morningllvm.hpp"

auto main() -> int {
    const std::string PROGRAM = R"(
[object Figure empty
    [scope
        (var x 0)
        (var y 0)
        (var width 0)
        (var height 0)

        (method create(this x y width height)
            [scope
                (set (property this x) x)
                (set (property this y) y)
                (set (property this width) width)
                (set (property this height) height)
            ]
        )

        (method areasize(this)
            (* (property this width) (property this height))
        )
    ]
]

(var square (newobj Figure 0 0 100 100))
(fprint "Square %d x %d" [property square width] [property square height])

    )";

    MorningLanguageLLVM morning_vm;

    morning_vm.execute(PROGRAM);

    return 0;
}
