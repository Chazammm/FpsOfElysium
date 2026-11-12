#pragma once
// Record causes in memory; the plugin emits each category at most once.
typedef enum {
    FPS_NO_CONTEXT,FPS_CONTEXT_CHANGED,FPS_GL_VERSION,FPS_CORE_PROFILE,
    FPS_ENTRY_POINT,FPS_SHADER_COMPILE,FPS_SHADER_LINK,FPS_CUSTOM_PROGRAM,
    FPS_TEXTURE_UNIT,FPS_PIXEL_BUFFER,FPS_FRAMEBUFFER,FPS_PIXEL_TRANSFER,
    FPS_SAMPLER_OBJECT,FPS_FIXED_FUNCTION,FPS_EXTRA_TEXTURE_UNIT,FPS_TEXTURE_MATRIX,
    FPS_RENDER_MODE,FPS_VBO_BOUND,FPS_VAO_BOUND,FPS_TEXTURE_SIZE,FPS_TEXTURE_FORMAT,
    FPS_TEXTURE_SAMPLING,FPS_TEXTURE_LEVELS,FPS_ALLOCATION,FPS_CACHE_LIMIT,
    FPS_CACHE_TABLE_FULL,FPS_BATCH_REPLAY,FPS_REASON_COUNT
} FpsFallbackReason;
_Static_assert(FPS_REASON_COUNT<=32,"diagnostic mask capacity");
static _Thread_local unsigned fps_fallback_reasons;
static void fps_note_fallback(FpsFallbackReason reason) {fps_fallback_reasons|=1u<<(unsigned)reason;}
static inline const char *fps_fallback_name(FpsFallbackReason reason) {
    static const char *const names[]={
        "no_current_context","context_changed","opengl_version_unsupported","compatibility_profile_required",
        "required_entry_point_missing","shader_compile_failed","shader_link_failed","foreign_shader_active",
        "nondefault_texture_unit","pixel_buffer_bound","framebuffer_bound","pixel_transfer_state",
        "sampler_object_bound","unsupported_fixed_function_state","additional_texture_unit_enabled","texture_matrix_changed",
        "render_mode_or_display_list","vertex_buffer_bound","vertex_array_object_bound","texture_size_unsupported","texture_format_unsupported",
        "texture_sampling_unsupported","texture_mip_chain_unsupported","texture_allocation_failed","texture_cache_limit",
        "texture_cache_table_full","buffered_batch_replayed"
    };
    return (unsigned)reason<FPS_REASON_COUNT?names[reason]:"unknown";
}
