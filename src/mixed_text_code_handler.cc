#include "mixed_text_code_handler.h"

namespace sentencepiece {

bool MixedTextCodeIterator::HasCodeHeader() const {
  return code_meta_block_begin_ >= 0 && head_ == cache_value_.data() && head_ != tail_ && *head_ == code_meta_block_begin_;
}

bool MixedTextCodeIterator::ReadCodeHeader(absl::string_view* line) {
  assert(*head_ == code_meta_block_begin_);
  assert(code_meta_block_end_ >= 0);
  auto ptr = reinterpret_cast<const char *>(memchr(head_, code_meta_block_end_, cache_value_.length()));
  assert((void("Code meta block did not end with code meta block end character"), ptr != nullptr));
  if (ptr != nullptr) {
    *line = absl::string_view(head_ + 1, ptr - head_ - 1);
    head_ = ptr + 1;
    return true;
  } else {
    head_ = tail_;
    return false;
  }
}

bool MixedTextCodeIterator::ReadTextBlock(absl::string_view* line) {
  const char* ptr;
  if (verbatim_control_char_ >= 0) {
    ptr = reinterpret_cast<const char *>(memchr(head_, verbatim_control_char_, cache_value_.size()));
  } else {
    ptr = nullptr;
  }
  if (ptr == nullptr) {
    *line = absl::string_view(head_, tail_ - head_);
    head_ = tail_;
  } else {
    *line = absl::string_view(head_, ptr - head_);
    head_ = ptr;
    in_text_ = false;
  }
  return line->size() > 0;
}

bool MixedTextCodeIterator::ReadCodeBlock(absl::string_view* line) {
  assert(*head_ == verbatim_control_char_);
  auto ptr = reinterpret_cast<const char *>(memchr(head_, code_block_end_, cache_value_.size()));
  assert((void("Code block does not end with code block end character"), ptr != nullptr));
  *line = absl::string_view(head_, ptr - head_);
  head_ = ptr + 1;
  in_text_ = true;
  return line->size() > 1; // skip x01
}

bool MixedTextCodeIterator::TryReadNext(absl::string_view* line) {
  if (HasCodeHeader()) {
    return ReadCodeHeader(line);
  } else if (in_text_) {
    // Expecting text block
    return ReadTextBlock(line);
  } else {
    // Expecting code block
    return ReadCodeBlock(line);
  }
}


MixedTextCodeIterator::MixedTextCodeIterator(absl::string_view cache_value,
  int32 verbatim_control_char,
  int32 code_block_end,
  int32 code_meta_block_begin,
  int32 code_meta_block_end
):
cache_value_(cache_value),
in_text_(true),
head_(cache_value_.data()),
tail_(cache_value_.data() + cache_value.size()),
verbatim_control_char_(verbatim_control_char),
code_block_end_(code_block_end),
code_meta_block_begin_(code_meta_block_begin),
code_meta_block_end_(code_meta_block_end)
{
}

bool MixedTextCodeIterator::Next(absl::string_view* line) {
  while (HasNext()) {
    // Skips empty blocks
    if (TryReadNext(line)) {
      return true;
    }
  }
  return false;
}

bool MixedTextCodeIterator::HasNext() const {
  return head_ < tail_;
}

}  // namespace sentencepiece