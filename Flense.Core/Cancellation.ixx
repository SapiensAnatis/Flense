export module Flense.Core.Shared:Cancellation;

import std;

export namespace Flense::Core
{
    /// <summary>
    /// Thrown when an operation stops early because its stop token was triggered. Not a failure, so it deliberately
    /// shares no base with the modules' error types beyond std::exception.
    /// </summary>
    class OperationCancelledError : public std::exception
    {
      public:
        [[nodiscard]] const char* what() const noexcept override
        {
            return "operation cancelled";
        }
    };
} // namespace Flense::Core
