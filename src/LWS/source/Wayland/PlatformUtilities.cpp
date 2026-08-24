#ifdef LWS_PLATFORM_WAYLAND
#include <LWS/Bitmap.hpp>
#include <LWS/Clipboard.hpp>
#include <LWS/FileDialog.hpp>
#include <LWS/NotificationIconGroup.hpp>

namespace LWS
{
    class Bitmap::Impl {};
    class NotificationIconGroup::Impl {};

    Bitmap::Bitmap(const BitmapBuffer&) {}
    Bitmap::Bitmap(const std::filesystem::path&) {}
    Bitmap::~Bitmap() = default;
    BitmapSharedPtr Bitmap::resize(int, int, uint8_t) const { return {}; }
    void Bitmap::SaveToFile(const std::filesystem::path&) const {}
    BitmapBuffer Bitmap::GetBitmapHeader() const { return {}; }
    Handle Bitmap::GetNativeHandle() const { return 0; }

    void Clipboard::RegisterFormat(ClipboardFormatType format) { fListFormats.push_back(format); }
    ClipboardFormatType Clipboard::RegisterFormat(const string_type&) { return 0; }
    ClipboardResult Clipboard::SetClipboardData(Handle, ClipboardFormatType, const LLUtils::Buffer&) { return ClipboardResult::UnknownError; }
    ClipboardResult Clipboard::SetClipboardData(Handle, ClipboardFormatType, const std::byte*, size_t) { return ClipboardResult::UnknownError; }
    ClipboardResult Clipboard::SetClipboardData(Handle, std::span<const ClipboardDataView>) { return ClipboardResult::UnknownError; }
    ClipboardResult Clipboard::SetClipboardText(Handle, const char_type*) { return ClipboardResult::UnknownError; }
    ClipboardData Clipboard::GetClipboardData() { return {}; }
    ClipboardResult Clipboard::GetClipboardError() const { return ClipboardResult::UnknownError; }

    FileDialogFilterBuilder::FileDialogFilterBuilder(const ListFileDialogFilters& filters) : fFilters(filters) {}
    const FileDialogFilterBuilder::ListFileDialogFilters& FileDialogFilterBuilder::GetFilters() const { return fFilters; }
    FileDialogResult FileDialog::Show(FileDialogType, const FileDialogFilterBuilder::ListFileDialogFilters&, const file_dialog_string_type&, Handle, const file_dialog_string_type&, uint32_t, file_dialog_string_type, file_dialog_string_type&) { return FileDialogResult::UnknownError; }
    FileDialogResult FileDialog::Show(FileDialogType, const FileDialogFilterBuilder::ListFileDialogFilters&, const file_dialog_string_type&, Handle, const file_dialog_string_type&, uint32_t, file_dialog_string_type, ListFileDialogFileNames&) { return FileDialogResult::UnknownError; }

    NotificationIconGroup::NotificationIconGroup() = default;
    NotificationIconGroup::~NotificationIconGroup() = default;
    NotificationIconGroup::IconID NotificationIconGroup::AddIconResource(uint16_t, const string_type&) { return 0; }
    Rect NotificationIconGroup::GetIconRect(IconID) const { return {}; }

}
#endif
