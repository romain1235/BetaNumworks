apps += Files::App
app_headers += apps/files/app.h \
               apps/files/file_name_cell.h

SFLAGS += -DHAS_FILES

app_files_src = $(addprefix apps/files/,\
  app.cpp \
  main_controller.cpp \
    files_parameter_controller.cpp \
  file_name_cell.cpp \
  delete_pop_up_controller.cpp \
  \
)

# PointerTextTableCell is defined in apps/external; include it here to
# ensure the symbol is available when linking the Files app.
app_files_src += apps/external/pointer_text_table_cell.cpp

apps_src += $(app_files_src)

i18n_files += $(call i18n_without_universal_for,files/base)

$(eval $(call depends_on_image,apps/files/app.cpp,apps/files/files_icon.png))
