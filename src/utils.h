#pragma once

#include <exception>
#include <source_location>
#include <string_view>

#include <fmt/core.h>

#include <vulkan/vulkan.h>

template <typename S, typename... Args>
[[noreturn]] inline void FatalError(const S& format, Args&&... args)
{
	fmt::print(stderr, "fatal error: {}\n", fmt::format(format, args...));
	std::terminate();
}

inline void VkCheck(
	const std::string_view message,
	const std::source_location loc = std::source_location::current())
{
	FatalError("VK_CHECK failed: `{}` at {}:{}",
		       message, loc.file_name(), loc.line());
}

#define VK_CHECK(result) if ((result) != VK_SUCCESS) VkCheck(#result)