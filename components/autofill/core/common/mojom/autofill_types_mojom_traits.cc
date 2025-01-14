// Copyright 2016 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/autofill/core/common/mojom/autofill_types_mojom_traits.h"

#include "base/i18n/rtl.h"
#include "components/autofill/core/common/form_field_data.h"
#include "components/autofill/core/common/html_field_types.h"
#include "mojo/public/cpp/base/string16_mojom_traits.h"
#include "mojo/public/cpp/base/time_mojom_traits.h"
#include "third_party/abseil-cpp/absl/types/variant.h"
#include "ui/gfx/geometry/mojom/geometry_mojom_traits.h"
#include "url/mojom/origin_mojom_traits.h"
#include "url/mojom/url_gurl_mojom_traits.h"

namespace mojo {

// static
bool StructTraits<autofill::mojom::FrameTokenDataView, autofill::FrameToken>::
    Read(autofill::mojom::FrameTokenDataView data, autofill::FrameToken* out) {
  base::UnguessableToken token;
  if (!data.ReadToken(&token))
    return false;
  if (data.is_local())
    *out = autofill::LocalFrameToken(token);
  else
    *out = autofill::RemoteFrameToken(token);
  return true;
}

// static
bool StructTraits<autofill::mojom::FrameTokenWithPredecessorDataView,
                  autofill::FrameTokenWithPredecessor>::
    Read(autofill::mojom::FrameTokenWithPredecessorDataView data,
         autofill::FrameTokenWithPredecessor* out) {
  if (!data.ReadToken(&out->token))
    return false;
  out->predecessor = data.predecessor();
  return out->predecessor >= -1;
}

// static
bool StructTraits<autofill::mojom::FormRendererIdDataView,
                  autofill::FormRendererId>::
    Read(autofill::mojom::FormRendererIdDataView data,
         autofill::FormRendererId* out) {
  *out = autofill::FormRendererId(data.id());
  return true;
}

// static
bool StructTraits<autofill::mojom::FieldRendererIdDataView,
                  autofill::FieldRendererId>::
    Read(autofill::mojom::FieldRendererIdDataView data,
         autofill::FieldRendererId* out) {
  *out = autofill::FieldRendererId(data.id());
  return true;
}

// static
bool StructTraits<
    autofill::mojom::SelectOptionDataView,
    autofill::SelectOption>::Read(autofill::mojom::SelectOptionDataView data,
                                  autofill::SelectOption* out) {
  if (!data.ReadValue(&out->value))
    return false;
  if (!data.ReadContent(&out->content))
    return false;
  return true;
}

// static
autofill::mojom::SectionValueDataView::Tag
UnionTraits<autofill::mojom::SectionValueDataView,
            autofill::Section::SectionValue>::
    GetTag(const autofill::Section::SectionValue& r) {
  if (absl::holds_alternative<autofill::Section::Default>(r))
    return autofill::mojom::SectionValueDataView::Tag::kDefaultSection;
  if (absl::holds_alternative<autofill::Section::Autocomplete>(r)) {
    return autofill::mojom::SectionValueDataView::Tag::kAutocomplete;
  }
  if (absl::holds_alternative<autofill::Section::FieldIdentifier>(r))
    return autofill::mojom::SectionValueDataView::Tag::kFieldIdentifier;

  NOTREACHED();
  return autofill::mojom::SectionValueDataView::Tag::kDefaultSection;
}

// static
bool UnionTraits<autofill::mojom::SectionValueDataView,
                 autofill::Section::SectionValue>::
    Read(autofill::mojom::SectionValueDataView data,
         autofill::Section::SectionValue* out) {
  switch (data.tag()) {
    case autofill::mojom::SectionValueDataView::Tag::kDefaultSection:
      *out = autofill::Section::Default();
      break;
    case autofill::mojom::SectionValueDataView::Tag::kAutocomplete: {
      autofill::Section::Autocomplete autocomplete;
      if (!data.ReadAutocomplete(&autocomplete))
        return false;
      *out = std::move(autocomplete);
      break;
    }
    case autofill::mojom::SectionValueDataView::Tag::kFieldIdentifier: {
      autofill::Section::FieldIdentifier field_identifier;
      if (!data.ReadFieldIdentifier(&field_identifier))
        return false;
      *out = std::move(field_identifier);
      break;
    }
  }
  return true;
}

// static
bool StructTraits<autofill::mojom::SectionAutocompleteDataView,
                  autofill::Section::Autocomplete>::
    Read(autofill::mojom::SectionAutocompleteDataView data,
         autofill::Section::Autocomplete* out) {
  if (!data.ReadSection(&out->section))
    return false;
  if (!data.ReadHtmlFieldMode(&out->mode))
    return false;
  return true;
}

// static
bool StructTraits<autofill::mojom::SectionFieldIdentifierDataView,
                  autofill::Section::FieldIdentifier>::
    Read(autofill::mojom::SectionFieldIdentifierDataView data,
         autofill::Section::FieldIdentifier* out) {
  if (!data.ReadFieldName(&out->field_name))
    return false;
  out->local_frame_id = data.local_frame_id();
  if (!data.ReadFieldRendererId(&out->field_renderer_id))
    return false;
  return true;
}

// static
bool StructTraits<autofill::mojom::SectionDataView, autofill::Section>::Read(
    autofill::mojom::SectionDataView data,
    autofill::Section* out) {
  if (!data.ReadValue(&out->value_))
    return false;
  return true;
}

// static
bool StructTraits<autofill::mojom::AutocompleteParsingResultDataView,
                  autofill::AutocompleteParsingResult>::
    Read(autofill::mojom::AutocompleteParsingResultDataView data,
         autofill::AutocompleteParsingResult* out) {
  if (!data.ReadSection(&out->section))
    return false;
  if (!data.ReadMode(&out->mode))
    return false;
  if (!data.ReadFieldType(&out->field_type))
    return false;
  return true;
}

// static
bool StructTraits<
    autofill::mojom::FormFieldDataDataView,
    autofill::FormFieldData>::Read(autofill::mojom::FormFieldDataDataView data,
                                   autofill::FormFieldData* out) {
  {
    std::u16string label;
    if (!data.ReadLabel(&label)) {
      return false;
    }
    out->label = std::move(label);
  }
  {
    std::u16string name;
    if (!data.ReadName(&name)) {
      return false;
    }
    out->set_name(std::move(name));
  }
  {
    std::u16string id_attribute;
    if (!data.ReadIdAttribute(&id_attribute)) {
      return false;
    }
    out->id_attribute = std::move(id_attribute);
  }
  {
    std::u16string name_attribute;
    if (!data.ReadNameAttribute(&name_attribute)) {
      return false;
    }
    out->name_attribute = std::move(name_attribute);
  }
  {
    std::u16string value;
    if (!data.ReadValue(&value)) {
      return false;
    }
    out->set_value(std::move(value));
  }
  {
    std::u16string selected_text;
    if (!data.ReadSelectedText(&selected_text)) {
      return false;
    }
    out->selected_text = std::move(selected_text);
  }

  {
    autofill::FormControlType form_control_type;
    if (!data.ReadFormControlType(&form_control_type)) {
      return false;
    }
    out->set_form_control_type(std::move(form_control_type));
  }
  {
    std::string autocomplete_attribute;
    if (!data.ReadAutocompleteAttribute(&autocomplete_attribute)) {
      return false;
    }
    out->autocomplete_attribute = std::move(autocomplete_attribute);
  }
  {
    std::optional<autofill::AutocompleteParsingResult> parsed_autocomplete;
    if (!data.ReadParsedAutocomplete(&parsed_autocomplete)) {
      return false;
    }
    out->parsed_autocomplete = std::move(parsed_autocomplete);
  }

  {
    std::u16string placeholder;
    if (!data.ReadPlaceholder(&placeholder)) {
      return false;
    }
    out->placeholder = std::move(placeholder);
  }

  {
    std::u16string css_classes;
    if (!data.ReadCssClasses(&css_classes)) {
      return false;
    }
    out->css_classes = std::move(css_classes);
  }

  {
    std::u16string aria_label;
    if (!data.ReadAriaLabel(&aria_label)) {
      return false;
    }
    out->aria_label = std::move(aria_label);
  }

  {
    std::u16string aria_description;
    if (!data.ReadAriaDescription(&aria_description)) {
      return false;
    }
    out->aria_description = std::move(aria_description);
  }

  {
    autofill::Section section;
    if (!data.ReadSection(&section)) {
      return false;
    }
    out->section = std::move(section);
  }

  out->properties_mask = data.properties_mask();

  {
    autofill::FieldRendererId renderer_id;
    if (!data.ReadRendererId(&renderer_id)) {
      return false;
    }
    out->set_renderer_id(std::move(renderer_id));
  }

  {
    autofill::FormRendererId host_form_id;
    if (!data.ReadHostFormId(&host_form_id)) {
      return false;
    }
    out->host_form_id = std::move(host_form_id);
  }

  out->form_control_ax_id = data.form_control_ax_id();
  out->max_length = data.max_length();
  out->is_user_edited = data.is_user_edited();
  out->is_autofilled = data.is_autofilled();

  {
    autofill::FormFieldData::CheckStatus check_status;
    if (!data.ReadCheckStatus(&check_status)) {
      return false;
    }
    out->check_status = std::move(check_status);
  }

  out->is_focusable = data.is_focusable();
  out->is_visible = data.is_visible();
  out->should_autocomplete = data.should_autocomplete();

  {
    autofill::FormFieldData::RoleAttribute role;
    if (!data.ReadRole(&role)) {
      return false;
    }
    out->role = std::move(role);
  }

  {
    base::i18n::TextDirection text_direction;
    if (!data.ReadTextDirection(&text_direction)) {
      return false;
    }
    out->text_direction = std::move(text_direction);
  }

  out->is_enabled = data.is_enabled();
  out->is_readonly = data.is_readonly();
  {
    std::u16string user_input;
    if (!data.ReadUserInput(&user_input)) {
      return false;
    }
    out->user_input = std::move(user_input);
  }

  {
    std::vector<autofill::SelectOption> options;
    if (!data.ReadOptions(&options)) {
      return false;
    }
    out->options = std::move(options);
  }

  {
    autofill::FormFieldData::LabelSource label_source;
    if (!data.ReadLabelSource(&label_source)) {
      return false;
    }
    out->label_source = std::move(label_source);
  }

  {
    gfx::RectF bounds;
    if (!data.ReadBounds(&bounds)) {
      return false;
    }
    out->bounds = std::move(bounds);
  }

  {
    std::vector<autofill::SelectOption> datalist_options;
    if (!data.ReadDatalistOptions(&datalist_options)) {
      return false;
    }
    out->datalist_options = std::move(datalist_options);
  }

  out->force_override = data.force_override();

  return true;
}

// static
bool StructTraits<autofill::mojom::FormFieldData_FillDataDataView,
                  autofill::FormFieldData::FillData>::
    Read(autofill::mojom::FormFieldData_FillDataDataView data,
         autofill::FormFieldData::FillData* out) {
  if (!data.ReadValue(&out->value)) {
    return false;
  }
  if (!data.ReadSection(&out->section)) {
    return false;
  }
  if (!data.ReadRendererId(&out->renderer_id)) {
    return false;
  }
  if (!data.ReadHostFormId(&out->host_form_id)) {
    return false;
  }
  out->is_autofilled = data.is_autofilled();
  out->force_override = data.force_override();
  return true;
}

// static
bool StructTraits<autofill::mojom::ButtonTitleInfoDataView,
                  autofill::ButtonTitleInfo>::
    Read(autofill::mojom::ButtonTitleInfoDataView data,
         autofill::ButtonTitleInfo* out) {
  return data.ReadTitle(&out->first) && data.ReadType(&out->second);
}

// static
bool StructTraits<autofill::mojom::FormDataDataView, autofill::FormData>::Read(
    autofill::mojom::FormDataDataView data,
    autofill::FormData* out) {
  if (!data.ReadIdAttribute(&out->id_attribute))
    return false;
  if (!data.ReadNameAttribute(&out->name_attribute))
    return false;
  if (!data.ReadName(&out->name))
    return false;
  if (!data.ReadButtonTitles(&out->button_titles))
    return false;
  if (!data.ReadAction(&out->action))
    return false;
  out->is_action_empty = data.is_action_empty();

  if (!data.ReadRendererId(&out->renderer_id)) {
    return false;
  }

  if (!data.ReadChildFrames(&out->child_frames))
    return false;

  if (!data.ReadSubmissionEvent(&out->submission_event))
    return false;

  if (!data.ReadFields(&out->fields))
    return false;

  if (!data.ReadUsernamePredictions(&out->username_predictions))
    return false;

  out->is_gaia_with_skip_save_password_form =
      data.is_gaia_with_skip_save_password_form();

  return base::ranges::all_of(
      out->child_frames,
      [&](int predecessor) {
        return predecessor == -1 ||
               base::checked_cast<size_t>(predecessor) < out->fields.size();
      },
      &autofill::FrameTokenWithPredecessor::predecessor);
}

// static
bool StructTraits<autofill::mojom::FormFieldDataPredictionsDataView,
                  autofill::FormFieldDataPredictions>::
    Read(autofill::mojom::FormFieldDataPredictionsDataView data,
         autofill::FormFieldDataPredictions* out) {
  if (!data.ReadHostFormSignature(&out->host_form_signature))
    return false;
  if (!data.ReadSignature(&out->signature))
    return false;
  if (!data.ReadHeuristicType(&out->heuristic_type))
    return false;
  if (!data.ReadServerType(&out->server_type))
    return false;
  if (!data.ReadHtmlType(&out->html_type)) {
    return false;
  }
  if (!data.ReadOverallType(&out->overall_type))
    return false;
  if (!data.ReadParseableName(&out->parseable_name))
    return false;
  if (!data.ReadSection(&out->section))
    return false;
  out->rank = data.rank();
  out->rank_in_signature_group = data.rank_in_signature_group();
  out->rank_in_host_form = data.rank_in_host_form();
  out->rank_in_host_form_signature_group =
      data.rank_in_host_form_signature_group();

  return true;
}

// static
bool StructTraits<autofill::mojom::FormDataPredictionsDataView,
                  autofill::FormDataPredictions>::
    Read(autofill::mojom::FormDataPredictionsDataView data,
         autofill::FormDataPredictions* out) {
  if (!data.ReadData(&out->data))
    return false;
  if (!data.ReadSignature(&out->signature))
    return false;
  if (!data.ReadAlternativeSignature(&out->alternative_signature)) {
    return false;
  }
  if (!data.ReadFields(&out->fields))
    return false;

  return true;
}

// static
bool StructTraits<autofill::mojom::PasswordAndMetadataDataView,
                  autofill::PasswordAndMetadata>::
    Read(autofill::mojom::PasswordAndMetadataDataView data,
         autofill::PasswordAndMetadata* out) {
  if (!data.ReadUsernameValue(&out->username_value)) {
    return false;
  }
  if (!data.ReadPasswordValue(&out->password_value)) {
    return false;
  }
  if (!data.ReadRealm(&out->realm))
    return false;

  out->uses_account_store = data.uses_account_store();

  return true;
}

// static
bool StructTraits<autofill::mojom::PasswordFormFillDataDataView,
                  autofill::PasswordFormFillData>::
    Read(autofill::mojom::PasswordFormFillDataDataView data,
         autofill::PasswordFormFillData* out) {
  if (!data.ReadFormRendererId(&out->form_renderer_id) ||
      !data.ReadUrl(&out->url) ||
      !data.ReadUsernameElementRendererId(&out->username_element_renderer_id) ||
      !data.ReadPasswordElementRendererId(&out->password_element_renderer_id) ||
      !data.ReadPreferredLogin(&out->preferred_login) ||
      !data.ReadAdditionalLogins(&out->additional_logins)) {
    return false;
  }

  out->wait_for_username = data.wait_for_username();
  out->username_may_use_prefilled_placeholder =
      data.username_may_use_prefilled_placeholder();

  return true;
}

// static
bool StructTraits<autofill::mojom::PasswordFormGenerationDataDataView,
                  autofill::PasswordFormGenerationData>::
    Read(autofill::mojom::PasswordFormGenerationDataDataView data,
         autofill::PasswordFormGenerationData* out) {
  return data.ReadNewPasswordRendererId(&out->new_password_renderer_id) &&
         data.ReadConfirmationPasswordRendererId(
             &out->confirmation_password_renderer_id);
}

// static
bool StructTraits<autofill::mojom::PasswordGenerationUIDataDataView,
                  autofill::password_generation::PasswordGenerationUIData>::
    Read(autofill::mojom::PasswordGenerationUIDataDataView data,
         autofill::password_generation::PasswordGenerationUIData* out) {
  if (!data.ReadBounds(&out->bounds))
    return false;

  out->max_length = data.max_length();
  out->is_generation_element_password_type =
      data.is_generation_element_password_type();
  out->input_field_empty = data.input_field_empty();

  return data.ReadGenerationElementId(&out->generation_element_id) &&
         data.ReadGenerationElement(&out->generation_element) &&
         data.ReadTextDirection(&out->text_direction) &&
         data.ReadFormData(&out->form_data);
}

// static
bool StructTraits<autofill::mojom::PasswordSuggestionRequestDataView,
                  autofill::PasswordSuggestionRequest>::
    Read(autofill::mojom::PasswordSuggestionRequestDataView data,
         autofill::PasswordSuggestionRequest* out) {
  out->username_field_index = data.username_field_index();
  out->password_field_index = data.password_field_index();
  out->show_webauthn_credentials = data.show_webauthn_credentials();

  return data.ReadElementId(&out->element_id) &&
         data.ReadFormData(&out->form_data) &&
         data.ReadTriggerSource(&out->trigger_source) &&
         data.ReadTextDirection(&out->text_direction) &&
         data.ReadTypedUsername(&out->typed_username) &&
         data.ReadBounds(&out->bounds);
}

bool StructTraits<
    autofill::mojom::ParsingResultDataView,
    autofill::ParsingResult>::Read(autofill::mojom::ParsingResultDataView data,
                                   autofill::ParsingResult* out) {
  return data.ReadUsernameRendererId(&out->username_renderer_id) &&
         data.ReadPasswordRendererId(&out->password_renderer_id) &&
         data.ReadNewPasswordRendererId(&out->new_password_renderer_id) &&
         data.ReadConfirmPasswordRendererId(&out->confirm_password_renderer_id);
}

}  // namespace mojo
