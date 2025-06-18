#include <string>

#include "morningllvm.hpp"

auto main() -> int {
    const std::string PROGRAM = R"(
[class Figure self
    [scope
        (var width 0)
        (var height 0)

        (func init (this width height)
            [scope
                0
                // (set (property this width) width)
                // (set (property this height) height)
            ]
        )

        (func area (this)
            0
            // (+ (property this width) (property this height))
        )
    ]
]

(var square (newobj Figure 10 10))
// (fprint "Square %d X %d" (property square width) (property square height))
    )";

    MorningLanguageLLVM morning_vm;

    morning_vm.execute(PROGRAM);

    return 0;
}
