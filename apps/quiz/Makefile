apps += Quiz::App
app_headers += apps/quiz/app.h 

SFLAGS += -DHAS_QUIZ

app_quiz_src = $(addprefix apps/quiz/, \
  app.cpp \
)

app_quiz_src += apps/quiz/main_controller.cpp

app_files_src += apps/external/pointer_text_table_cell.cpp

apps_src += $(app_quiz_src)

i18n_files += $(call i18n_without_universal_for,quiz/base)

$(eval $(call depends_on_image,apps/quiz/app.cpp,apps/quiz/quiz_icon.png))
