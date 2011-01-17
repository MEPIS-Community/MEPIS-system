QTDIR = /usr/local/qt4
TEMPLATE = app
TARGET = msystem
TRANSLATIONS += msystem_ar.ts msystem_ca.ts msystem_de.ts msystem_el.ts \
  msystem_es.ts msystem_fr.ts msystem_hi.ts msystem_it.ts msystem_ja.ts \
  msystem_ko.ts msystem_nl.ts msystem_pl.ts msystem_pt.ts msystem_pt_BR.ts \ 
  msystem_zh_CN.ts msystem_zh_TW.ts 
FORMS += meconfig.ui 
HEADERS += mconfig.h 
SOURCES += main.cpp mconfig.cpp 
CONFIG += release warn_on thread qt
