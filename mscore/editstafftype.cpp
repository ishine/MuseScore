//=============================================================================
//  MuseScore
//  Music Composition & Notation
//
//  Copyright (C) 2010-2014 Werner Schweer and others
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2
//  as published by the Free Software Foundation and appearing in
//  the file LICENCE.GPL
//=============================================================================

#include "editstafftype.h"
#include "libmscore/part.h"
#include "libmscore/score.h"
#include "libmscore/staff.h"
#include "libmscore/stringdata.h"
#include "musescore.h"
#include "navigator.h"
#include "scoreview.h"

namespace Ms {

extern Score::FileError readScore(Score* score, QString name, bool ignoreVersionError);

const QString g_groupNames[STAFF_GROUP_MAX] = {
      QString(QT_TRANSLATE_NOOP("staff group header name", "STANDARD STAFF")),
      QString(QT_TRANSLATE_NOOP("staff group header name", "PERCUSSION STAFF")),
      QString(QT_TRANSLATE_NOOP("staff group header name", "TABLATURE STAFF"))
};

//---------------------------------------------------------
//   EditStaffType
//---------------------------------------------------------

EditStaffType::EditStaffType(QWidget* parent, Staff* st)
   : QDialog(parent)
      {
      setWindowFlags(this->windowFlags() & ~Qt::WindowContextHelpButtonHint);
      setupUi(this);

      QButtonGroup* bg1 = new QButtonGroup(this);
      bg1->addButton(numbersRadio);
      bg1->addButton(lettersRadio);

      QButtonGroup* bg2 = new QButtonGroup(this);
      bg2->addButton(onLinesRadio);
      bg2->addButton(aboveLinesRadio);

      QButtonGroup* bg3 = new QButtonGroup(this);
      bg3->addButton(linesThroughRadio);
      bg3->addButton(linesBrokenRadio);

      staff     = st;
      staffType = *staff->staffType();
      Instrument* instr = staff->part()->instr();

      // template combo

      templateCombo->clear();
      // statdard group also as fall-back (but excluded by percussion)
      bool bStandard = !(instr != nullptr && instr->drumset() != nullptr);
      bool bPerc        = (instr != nullptr && instr->drumset() != nullptr);
      bool bTab         = (instr != nullptr && instr->stringData() != nullptr && instr->stringData()->strings() > 0);
      int idx           = 0;
      for (const StaffType& t : StaffType::presets()) {
            if ( (t.group() == STANDARD_STAFF_GROUP && bStandard)
                        || (t.group() == PERCUSSION_STAFF_GROUP && bPerc)
                        || (t.group() == TAB_STAFF_GROUP && bTab))
                  templateCombo->addItem(t.name(), idx);
            idx++;
            }

      // tab page configuration
      QList<QString> fontNames = StaffType::fontNames(false);
      foreach (const QString& name, fontNames)   // fill fret font name combo
            fretFontName->addItem(name);
      fretFontName->setCurrentIndex(0);
      fontNames = StaffType::fontNames(true);
      foreach(const QString& name, fontNames)   // fill duration font name combo
            durFontName->addItem(name);
      durFontName->setCurrentIndex(0);

      // load a sample tabulature score in preview
      Score* sc = new Score(MScore::defaultStyle());
      if (readScore(sc, QString(":/data/tab_sample.mscx"), false) == Score::FileError::FILE_NO_ERROR)
            preview->setScore(sc);

      setValues();

      connect(name,           SIGNAL(textEdited(const QString&)), SLOT(nameEdited(const QString&)));
      connect(lines,          SIGNAL(valueChanged(int)),          SLOT(updatePreview()));
      connect(lineDistance,   SIGNAL(valueChanged(double)),       SLOT(updatePreview()));
      connect(showBarlines,   SIGNAL(toggled(bool)),              SLOT(updatePreview()));
      connect(genClef,        SIGNAL(toggled(bool)),              SLOT(updatePreview()));
      connect(genTimesig,     SIGNAL(toggled(bool)),              SLOT(updatePreview()));

      connect(genKeysigPercussion,       SIGNAL(toggled(bool)),   SLOT(updatePreview()));
      connect(showLedgerLinesPercussion, SIGNAL(toggled(bool)),   SLOT(updatePreview()));
      connect(stemlessPercussion,        SIGNAL(toggled(bool)),   SLOT(updatePreview()));

      connect(noteValuesSymb, SIGNAL(toggled(bool)),              SLOT(updatePreview()));
      connect(noteValuesStems,SIGNAL(toggled(bool)),              SLOT(tabStemsToggled(bool)));
      connect(stemBesideRadio,SIGNAL(toggled(bool)),              SLOT(updatePreview()));
      connect(stemThroughRadio,SIGNAL(toggled(bool)),             SLOT(tabStemThroughToggled(bool)));
      connect(stemAboveRadio, SIGNAL(toggled(bool)),              SLOT(updatePreview()));
      connect(stemBelowRadio, SIGNAL(toggled(bool)),              SLOT(updatePreview()));
      connect(minimShortRadio,SIGNAL(toggled(bool)),              SLOT(tabMinimShortToggled(bool)));
      connect(minimSlashedRadio,SIGNAL(toggled(bool)),            SLOT(updatePreview()));
      connect(showRests,      SIGNAL(toggled(bool)),              SLOT(updatePreview()));
      connect(durFontName,    SIGNAL(currentIndexChanged(int)),   SLOT(durFontNameChanged(int)));
      connect(durFontSize,    SIGNAL(valueChanged(double)),       SLOT(updatePreview()));
      connect(durY,           SIGNAL(valueChanged(double)),       SLOT(updatePreview()));
      connect(fretFontName,   SIGNAL(currentIndexChanged(int)),   SLOT(fretFontNameChanged(int)));
      connect(fretFontSize,   SIGNAL(valueChanged(double)),       SLOT(updatePreview()));
      connect(fretY,          SIGNAL(valueChanged(double)),       SLOT(updatePreview()));

      connect(linesThroughRadio, SIGNAL(toggled(bool)),           SLOT(updatePreview()));
      connect(onLinesRadio,   SIGNAL(toggled(bool)),              SLOT(updatePreview()));
      connect(upsideDown,     SIGNAL(toggled(bool)),              SLOT(updatePreview()));
      connect(numbersRadio,   SIGNAL(toggled(bool)),              SLOT(updatePreview()));

      connect(templateReset,  SIGNAL(clicked()),                  SLOT(resetToTemplateClicked()));
      connect(addToTemplates,   SIGNAL(clicked()),                SLOT(addToTemplatesClicked()));
//      connect(groupCombo,       SIGNAL(currentIndexChanged(int)), SLOT(staffGroupChanged(int)));
      }

//---------------------------------------------------------
//   staffGroupChanged
//---------------------------------------------------------
/*
void EditStaffType::staffGroupChanged(int n)
      {
      int groupIdx = groupCombo->itemData(groupCombo->currentIndex()).toInt();
      StaffGroup group = StaffGroup(groupIdx);
      staffType = *StaffType::getDefaultPreset(group); // overwrite with default
      setValues();
      }
*/
//---------------------------------------------------------
//   setValues
//---------------------------------------------------------

void EditStaffType::setValues()
      {
      blockSignals(true);

      StaffGroup group = staffType.group();
      int idx = int(group);
      stack->setCurrentIndex(idx);
      groupName->setText(g_groupNames[idx]);
//      groupCombo->setCurrentIndex(idx);

      name->setText(staffType.name());
      lines->setValue(staffType.lines());
      lineDistance->setValue(staffType.lineDistance().val());
      genClef->setChecked(staffType.genClef());
      showBarlines->setChecked(staffType.showBarlines());
      genTimesig->setChecked(staffType.genTimesig());

      switch (group) {
            case StaffGroup::STANDARD:
                  genKeysigPitched->setChecked(staffType.genKeysig());
                  showLedgerLinesPitched->setChecked(staffType.showLedgerLines());
                  stemlessPitched->setChecked(staffType.slashStyle());
                  break;

            case StaffGroup::TAB:
                  {
                  upsideDown->setChecked(staffType.upsideDown());
                  int idx = fretFontName->findText(staffType.fretFontName(), Qt::MatchFixedString);
                  if (idx == -1)
                        idx = 0;          // if name not found, use first name
                  fretFontName->setCurrentIndex(idx);
                  fretFontSize->setValue(staffType.fretFontSize());
                  fretY->setValue(staffType.fretFontUserY());

                  numbersRadio->setChecked(staffType.useNumbers());
                  onLinesRadio->setChecked(staffType.onLines());
                  linesThroughRadio->setChecked(staffType.linesThrough());
                  linesBrokenRadio->setChecked(!staffType.linesThrough());

                  idx = durFontName->findText(staffType.durationFontName(), Qt::MatchFixedString);
                  if (idx == -1)
                        idx = 0;          // if name not found, use first name
                  durFontName->setCurrentIndex(idx);
                  durFontSize->setValue(staffType.durationFontSize());
                  durY->setValue(staffType.durationFontUserY());
                  // convert combined values of genDurations and slashStyle into noteValuesx radio buttons
                  // Sbove/Below, Beside/Through and minim are only used if stems-and-beams
                  // but set them from stt values anyway, to ensure preset matching
                  stemAboveRadio->setChecked(!staffType.stemsDown());
                  stemBelowRadio->setChecked(staffType.stemsDown());
                  stemBesideRadio->setChecked(!staffType.stemThrough());
                  stemThroughRadio->setChecked(staffType.stemThrough());
                  TablatureMinimStyle minimStyle = staffType.minimStyle();
                  minimNoneRadio->setChecked(minimStyle == TablatureMinimStyle::NONE);
                  minimShortRadio->setChecked(minimStyle == TablatureMinimStyle::SHORTER);
                  minimSlashedRadio->setChecked(minimStyle == TablatureMinimStyle::SLASHED);
                  if (staffType.genDurations()) {
                        noteValuesNone->setChecked(false);
                        noteValuesSymb->setChecked(true);
                        noteValuesStems->setChecked(false);
                        }
                  else {
                        if (staffType.slashStyle()) {
                              noteValuesNone->setChecked(true);
                              noteValuesSymb->setChecked(false);
                              noteValuesStems->setChecked(false);
                              }
                        else {
                              noteValuesNone->setChecked(false);
                              noteValuesSymb->setChecked(false);
                              noteValuesStems->setChecked(true);
                              }
                        }
                  showRests->setChecked(staffType.showRests());
                  // adjust compatibility across different settings
                  tabStemThroughCompatibility(stemThroughRadio->isChecked());
                  tabMinimShortCompatibility(minimShortRadio->isChecked());
                  tabStemsCompatibility(noteValuesStems->isChecked());
                  }
                  break;

            case StaffGroup::PERCUSSION:
                  genKeysigPercussion->setChecked(staffType.genKeysig());
                  showLedgerLinesPercussion->setChecked(staffType.showLedgerLines());
                  stemlessPercussion->setChecked(staffType.slashStyle());
                  break;
            }
      updatePreview();
      blockSignals(false);
      }

//---------------------------------------------------------
//   nameEdited
//---------------------------------------------------------

void EditStaffType::nameEdited(const QString& /*s*/)
      {
//      staffTypeList->currentItem()->setText(s);
      }

//=========================================================
//   PERCUSSION PAGE METHODS
//=========================================================


//=========================================================
//   TABULATURE PAGE METHODS
//=========================================================

//---------------------------------------------------------
//   Tabulature duration / fret font name changed
//
//    set depending parameters
//---------------------------------------------------------

void EditStaffType::durFontNameChanged(int idx)
      {
      qreal size, yOff;
      if (StaffType::fontData(true, idx, 0, 0, &size, &yOff)) {
            durFontSize->setValue(size);
            durY->setValue(yOff);
            }
      updatePreview();
      }

void EditStaffType::fretFontNameChanged(int idx)
      {
      qreal size, yOff;
      if (StaffType::fontData(false, idx, 0, 0, &size, &yOff)) {
            fretFontSize->setValue(size);
            fretY->setValue(yOff);
            }
      updatePreview();
      }

//---------------------------------------------------------
//   Tabulature note stems toggled
//
//    enable / disable all controls related to stems
//---------------------------------------------------------

void EditStaffType::tabStemsToggled(bool checked)
      {
      tabStemsCompatibility(checked);
      updatePreview();
      }

//---------------------------------------------------------
//   Tabulature "minim short" toggled
//
//    contra-toggle "stems through"
//---------------------------------------------------------

void EditStaffType::tabMinimShortToggled(bool checked)
      {
      tabMinimShortCompatibility(checked);
      updatePreview();
      }

//---------------------------------------------------------
//   Tabulature "stems through" toggled
//---------------------------------------------------------

void EditStaffType::tabStemThroughToggled(bool checked)
      {
      tabStemThroughCompatibility(checked);
      updatePreview();
      }

//---------------------------------------------------------
//   setFromDlg
//
//    initializes a StaffType from dlg controls
//---------------------------------------------------------

void EditStaffType::setFromDlg()
      {
      staffType.setName(name->text());
      staffType.setLines(lines->value());
      staffType.setLineDistance(Spatium(lineDistance->value()));
      staffType.setGenClef(genClef->isChecked());
      staffType.setShowBarlines(showBarlines->isChecked());
      staffType.setGenTimesig(genTimesig->isChecked());
      if (staffType.group() == StaffGroup::STANDARD) {
            staffType.setGenKeysig(genKeysigPitched->isChecked());
            staffType.setShowLedgerLines(showLedgerLinesPitched->isChecked());
            staffType.setSlashStyle(stemlessPitched->isChecked());
            }
      if (staffType.group() == StaffGroup::PERCUSSION) {
            staffType.setGenKeysig(genKeysigPercussion->isChecked());
            staffType.setShowLedgerLines(showLedgerLinesPercussion->isChecked());
            staffType.setSlashStyle(stemlessPercussion->isChecked());
            }
      staffType.setDurationFontName(durFontName->currentText());
      staffType.setDurationFontSize(durFontSize->value());
      staffType.setDurationFontUserY(durY->value());
      staffType.setFretFontName(fretFontName->currentText());
      staffType.setFretFontSize(fretFontSize->value());
      staffType.setFretFontUserY(fretY->value());
      staffType.setLinesThrough(linesThroughRadio->isChecked());
      staffType.setMinimStyle(minimNoneRadio->isChecked() ? TablatureMinimStyle::NONE :
            (minimShortRadio->isChecked() ? TablatureMinimStyle::SHORTER : TablatureMinimStyle::SLASHED));
      staffType.setOnLines(onLinesRadio->isChecked());
      staffType.setShowRests(showRests->isChecked());
      staffType.setUpsideDown(upsideDown->isChecked());
      staffType.setUseNumbers(numbersRadio->isChecked());
      //note values
      staffType.setStemsDown(stemBelowRadio->isChecked());
      staffType.setStemsThrough(stemThroughRadio->isChecked());
      if (staffType.group() == StaffGroup::TAB) {
            staffType.setSlashStyle(true);                 // assume no note values
            staffType.setGenDurations(false);              //    "     "
            if (noteValuesSymb->isChecked())
                  staffType.setGenDurations(true);
            if (noteValuesStems->isChecked())
                  staffType.setSlashStyle(false);
            }
      }

//---------------------------------------------------------
//   Block preview signals
//---------------------------------------------------------

void EditStaffType::blockSignals(bool block)
      {
      stack->blockSignals(block);
//      groupCombo->blockSignals(block);
      lines->blockSignals(block);
      lineDistance->blockSignals(block);
      showBarlines->blockSignals(block);
      genClef->blockSignals(block);
      genTimesig->blockSignals(block);
      noteValuesSymb->blockSignals(block);
      noteValuesStems->blockSignals(block);
      durFontName->blockSignals(block);
      durFontSize->blockSignals(block);
      durY->blockSignals(block);
      fretFontName->blockSignals(block);
      fretFontSize->blockSignals(block);
      fretY->blockSignals(block);

      numbersRadio->blockSignals(block);
      linesThroughRadio->blockSignals(block);
      onLinesRadio->blockSignals(block);

      upsideDown->blockSignals(block);
      stemAboveRadio->blockSignals(block);
      stemBelowRadio->blockSignals(block);
      stemBesideRadio->blockSignals(block);
      stemThroughRadio->blockSignals(block);
      minimShortRadio->blockSignals(block);
      minimSlashedRadio->blockSignals(block);
      showRests->blockSignals(block);

      showLedgerLinesPercussion->blockSignals(block);
      genKeysigPercussion->blockSignals(block);
      stemlessPercussion->blockSignals(block);
      }

//---------------------------------------------------------
//   Tabulature note stems compatibility
//
//    Enable / disable all stem-related controls according to "Stems and beams" is checked/unchecked
//---------------------------------------------------------

void EditStaffType::tabStemsCompatibility(bool checked)
      {
      stemAboveRadio->setEnabled(checked && !stemThroughRadio->isChecked());
      stemBelowRadio->setEnabled(checked && !stemThroughRadio->isChecked());
      stemBesideRadio->setEnabled(checked);
      stemThroughRadio->setEnabled(checked && !minimShortRadio->isChecked());
      minimNoneRadio->setEnabled(checked);
      minimShortRadio->setEnabled(checked && !stemThroughRadio->isChecked());
      minimSlashedRadio->setEnabled(checked);
      }

//---------------------------------------------------------
//   Tabulature "minim short" compatibility
//
//    Setting "short minim" stems is incompatible with "stems through":
//    if checked and "stems through" is checked, move check to "stems beside"
//---------------------------------------------------------

void EditStaffType::tabMinimShortCompatibility(bool checked)
      {
      if (checked) {
            if(stemThroughRadio->isChecked()) {
                  stemThroughRadio->setChecked(false);
                  stemBesideRadio->setChecked(true);
                  }
            }
      // disable / enable "stems through" according "minim short" is checked / unchecked
      stemThroughRadio->setEnabled(!checked && noteValuesStems->isChecked());
      }

//---------------------------------------------------------
//   Tabulature "stems through" compatibility
//
//    Setting "stems through" is incompatible with "minim short":
//    if checking and "minim short" is checked, move check to "minim slashed"
//    It also make "Stems above" and "Stems below" meaningless: disable them
//---------------------------------------------------------

void EditStaffType::tabStemThroughCompatibility(bool checked)
      {
      if (checked) {
            if(minimShortRadio->isChecked()) {
                  minimShortRadio->setChecked(false);
                  minimSlashedRadio->setChecked(true);
                  }
            }
      // disable / enable "minim short" and "stems above/below" according "stems through" is checked / unchecked
      bool enab = !checked && noteValuesStems->isChecked();
      minimShortRadio->setEnabled(enab);
      stemAboveRadio->setEnabled(enab);
      stemBelowRadio->setEnabled(enab);
      }

//---------------------------------------------------------
//    updatePreview
///   update staff type in preview score
//---------------------------------------------------------

void EditStaffType::updatePreview()
      {
      setFromDlg();
      if (preview) {
            preview->score()->staff(0)->setStaffType(&staffType);
            preview->score()->cmdUpdateNotes();
            preview->score()->doLayout();
            preview->updateAll();
            preview->update();
            }
      }

//---------------------------------------------------------
//   createUniqueStaffTypeName
///  create unique new name for StaffType
//---------------------------------------------------------

QString EditStaffType::createUniqueStaffTypeName(StaffGroup group)
      {
      QString name;
      for (int idx = 1; ; ++idx) {
            switch (group) {
                  case StaffGroup::STANDARD:
                        name = QString("Standard-%1 [*]").arg(idx);
                        break;
                  case StaffGroup::PERCUSSION:
                        name = QString("Perc-%1 [*]").arg(idx);
                        break;
                  case StaffGroup::TAB:
                        name = QString("Tab-%1 [*]").arg(idx);
                        break;
                  }
            bool found = false;
            for (const StaffType& st : StaffType::presets()) {
                  if (st.name() == name) {
                        found = true;
                        break;
                        }
                  }
            if (!found)
                  break;
            }
      return name;
      }

//---------------------------------------------------------
//   savePresets
//---------------------------------------------------------

void EditStaffType::savePresets()
      {
      printf("savePresets\n");
      }

//---------------------------------------------------------
//   loadPresets
//---------------------------------------------------------

void EditStaffType::loadPresets()
      {
      printf("loadPresets\n");
      }

//---------------------------------------------------------
//   loadFromTemplate
//---------------------------------------------------------
/*
void EditStaffType::loadFromTemplateClicked()
      {
      StaffTypeTemplates stt(staffType);
      if (stt.exec()) {
            StaffType* st = stt.staffType();
            staffType = *st;
            setValues();
            updatePreview();
            }
      }
*/
void EditStaffType::resetToTemplateClicked()
      {
      int idx = templateCombo->itemData(templateCombo->currentIndex()).toInt();
      staffType = *(StaffType::preset(idx));
      setValues();
      }

//---------------------------------------------------------
//   addToTemplates
//---------------------------------------------------------

void EditStaffType::addToTemplatesClicked()
      {
      qDebug("not implemented: add to templates");
      }

//---------------------------------------------------------
//   StaffTypeTemplates
//---------------------------------------------------------
/*
StaffTypeTemplates::StaffTypeTemplates(const StaffType& st, QWidget* parent)
   : QDialog(parent)
      {
      setupUi(this);

      QList<const StaffType*> stl;
      for (const StaffType& t : StaffType::presets()) {
            if (t.group() == st.group())
                  stl.append(&t);
            }
      for (const StaffType* t : stl) {
            QListWidgetItem* item = new QListWidgetItem(t->name());
            item->setData(Qt::UserRole,
               QVariant::fromValue<void*>((void*)t)
               );
            staffTypeList->addItem(item);
            }
      staffTypeList->setCurrentRow(0);
      }
*/
//---------------------------------------------------------
//   staffType
//---------------------------------------------------------
/*
StaffType* StaffTypeTemplates::staffType() const
      {
      return (StaffType*)staffTypeList->currentItem()->data(Qt::UserRole).value<void*>();
      }
*/
}

