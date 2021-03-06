/*
 * Copyright (C) 2006, 2007, 2008, 2009, 2010, 2011, 2012
 * Computer Graphics Group, University of Siegen, Germany.
 * Written by Martin Lambers <martin.lambers@uni-siegen.de>.
 * See http://www.cg.informatik.uni-siegen.de/ for contact information.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <sstream>
#include <cstdint>
#include <cmath>

#include <QApplication>
#include <QLabel>
#include <QMessageBox>
#include <QTabWidget>
#include <QTextEdit>
#include <QGridLayout>
#include <QPainter>
#include <QMenu>
#include <QAction>
#include <QCursor>
#include <QPushButton>
#include <QRadioButton>
#include <QColorDialog>
#include <QFileDialog>
#include <QSettings>
#include <QLabel>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QSlider>
#include <QComboBox>
#include <QGroupBox>
#include <QStackedWidget>

#include <ecmdb/ecmdb.h>

#include "dbg.h"
#include "exc.h"
#include "str.h"
#include "msg.h"
#include "fio.h"

#include "glvm.h"

#include "guitools.h"
#include "databases.h"

using namespace glvm;


DBInfo::DBInfo(const class database_description& dd, QWidget* parent) : QWidget(parent)
{
    QGridLayout* layout = new QGridLayout;

    QLabel* url_label = new QLabel("URL:");
    layout->addWidget(url_label, 0, 0);
    QLabel* url_content = new QLabel(toQString(str::asprintf("<a href=\"%s\">%s</a>", dd.url.c_str(), dd.url.c_str())));
    layout->addWidget(url_content, 0, 1);

    QLabel* desc_label = new QLabel("Description:");
    layout->addWidget(desc_label, 1, 0);
    QTextDocument* desc_content_doc = new QTextDocument(this);
    QString desc_content_str;
    for (size_t i = 0; i < dd.db.description().size(); i++) {
        desc_content_str += QString(dd.db.description()[i].c_str()) + QString("\n");
    }
    desc_content_doc->setPlainText(desc_content_str);
    QTextEdit* desc_content = new QTextEdit(this);
    desc_content->setDocument(desc_content_doc);
    desc_content->setReadOnly(true);
    desc_content->setLineWrapMode(QTextEdit::NoWrap);
    desc_content->setAutoFormatting(QTextEdit::AutoNone);
    desc_content->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    desc_content->setFixedHeight(desc_content->fontMetrics().lineSpacing() * 4 + desc_content->fontMetrics().lineSpacing() / 2);
    layout->addWidget(desc_content, 1, 1);

    QLabel* ellipsoid_label = new QLabel("Ellipsoid:");
    layout->addWidget(ellipsoid_label, 2, 0);
    QLabel* ellipsoid_content = new QLabel();
    if (equal(dd.db.semi_major_axis(), ecm::semi_major_axis_earth_wgs84)
            && equal(dd.db.semi_minor_axis(), ecm::semi_minor_axis_earth_wgs84)) {
        ellipsoid_content->setText("Earth WGS84");
    } else if (equal(dd.db.semi_major_axis(), ecm::radius_moon_nasa)
            && equal(dd.db.semi_minor_axis(), ecm::radius_moon_nasa)) {
        ellipsoid_content->setText("Moon NASA");
    } else if (equal(dd.db.semi_major_axis(), ecm::semi_major_axis_mars_nasa)
            && equal(dd.db.semi_minor_axis(), ecm::semi_minor_axis_mars_nasa)) {
        ellipsoid_content->setText("Mars NASA");
    } else {
        ellipsoid_content->setText(str::asprintf("a = %s, b = %s",
                        str::from(dd.db.semi_major_axis()).c_str(), str::from(dd.db.semi_minor_axis()).c_str()).c_str());
    }
    layout->addWidget(ellipsoid_content, 2, 1);

    QLabel* category_label = new QLabel("Category:");
    layout->addWidget(category_label, 3, 0);
    QLabel* category_content = new QLabel();
    if (dd.db.category() == ecmdb::category_elevation) {
        category_content->setText(str::asprintf("Elevation data, %s bits",
                    str::from(dd.db.type_size() * CHAR_BIT).c_str()).c_str());
    } else if (dd.db.category() == ecmdb::category_texture) {
        category_content->setText(str::asprintf("Image data, %s channels, %s bits per channel",
                    str::from(dd.db.channels()).c_str(), str::from(dd.db.type_size() * CHAR_BIT).c_str()).c_str());
    } else if (dd.db.category() == ecmdb::category_sar_amplitude) {
        category_content->setText("SAR amplitude data");
    } else if (dd.db.category() == ecmdb::category_data) {
        category_content->setText(str::asprintf("Scalar data, %s channels, %s bits per channel",
                    str::from(dd.db.channels()).c_str(), str::from(dd.db.type_size() * CHAR_BIT).c_str()).c_str());
    }
    layout->addWidget(category_content, 3, 1);

    QLabel* metadata_label = new QLabel("Metadata:");
    layout->addWidget(metadata_label, 4, 0);
    QLabel* metadata_content = new QLabel();
    if (dd.db.category() == ecmdb::category_elevation) {
        metadata_content->setText(str::asprintf("Elevation range [%s, %s]",
                    str::from(dd.meta.elevation.min).c_str(), str::from(dd.meta.elevation.max).c_str()).c_str());
    } else {
        metadata_content->setText("-");
    }
    layout->addWidget(metadata_content, 4, 1);

    QLabel* tile_label = new QLabel("Quad size:");
    layout->addWidget(tile_label, 5, 0);
    QLabel* tile_content = new QLabel(str::asprintf("%dx%d + a border of size %d",
                dd.db.quad_size(), dd.db.quad_size(), dd.db.overlap()).c_str());
    layout->addWidget(tile_content, 5, 1);

    layout->setRowStretch(6, 1);
    setLayout(layout);
}

DBProcessingParameters::DBProcessingParameters(QSettings *settings, bool lens, state* master_state, const uuid& db_uuid, QWidget* parent) :
    QWidget(parent), _settings(settings), _lock(false), _master_state(master_state), _db_uuid(db_uuid), _p_index(lens ? 1 : 0)
{
    QGridLayout* layout = new QGridLayout;

    _active_checkbox = new QCheckBox("Active");
    connect(_active_checkbox, SIGNAL(toggled(bool)), this, SLOT(set_active(bool)));
    layout->addWidget(_active_checkbox, 0, 0);

    QLabel* priority_label = new QLabel("Priority:");
    layout->addWidget(priority_label, 0, 2);
    _priority_spinbox = new QSpinBox();
    _priority_spinbox->setRange(1, 99);
    _priority_spinbox->setSingleStep(1);
    connect(_priority_spinbox, SIGNAL(valueChanged(int)), this, SLOT(set_priority(int)));
    layout->addWidget(_priority_spinbox, 0, 3);

    QLabel* weight_label = new QLabel("Weight:");
    layout->addWidget(weight_label, 0, 5);
    _weight_spinbox = new QDoubleSpinBox();
    _weight_spinbox->setRange(0.0, 1.0);
    _weight_spinbox->setSingleStep(0.1);
    connect(_weight_spinbox, SIGNAL(valueChanged(double)), this, SLOT(set_weight(double)));
    layout->addWidget(_weight_spinbox, 0, 6);

    if (lens) {
        _reset_button = new QPushButton("Set from global");
        layout->addWidget(_reset_button, 0, 8);
    } else {
        _reset_button = NULL;
        /* XXX:
         * Try to add a dummy widget with the same size that the "Reset" Button would have,
         * so that the layout of the Global and Lens parameter widgets is identical.
         * This is ugly and does not seem to work properly.  */
        QPushButton* dummy_button = new QPushButton("Set from global");
        layout->addWidget(dummy_button, 0, 8);
        QLabel* dummy_label = new QLabel("");
        layout->addWidget(dummy_label, 0, 8);
        dummy_label->setFixedSize(dummy_button->minimumSizeHint());
        dummy_button->hide();
    }

    bool is_e2c = _master_state->get_database_description(_db_uuid)->processing_parameters[0].category_e2c;
    ecmdb::category_t db_category = _master_state->get_database_description(_db_uuid)->db.category();

    QGroupBox* box = new QGroupBox(
            is_e2c ? "Texture from elevation parameters" :
            db_category == ecmdb::category_elevation ? "Elevation parameters" :
            db_category == ecmdb::category_texture ? "Image parameters" :
            db_category == ecmdb::category_sar_amplitude ? "SAR Amplitude Image parameters" :
            db_category == ecmdb::category_data ? "Scalar parameters" : "");
    layout->addWidget(box, 1, 0, 1, 9);
    layout->setRowStretch(2, 1);

    QGridLayout* box_layout = new QGridLayout;
    int row = 0;
    if (is_e2c) {
        QPushButton* e2c_gradient_pushbutton = new QPushButton("Color gradient:");
        connect(e2c_gradient_pushbutton, SIGNAL(pressed()), this, SLOT(set_e2c_gradient()));
        box_layout->addWidget(e2c_gradient_pushbutton, row, 0);
        _e2c_gradient_label = new QLabel();
        _e2c_gradient_label->setScaledContents(true);
        box_layout->addWidget(_e2c_gradient_label, row, 1);
        row++;
        QLabel* e2c_adapt_brightness_label = new QLabel("Adapt brightness:");
        box_layout->addWidget(e2c_adapt_brightness_label, row, 0);
        _e2c_adapt_brightness_checkbox = new QCheckBox(this);
        connect(_e2c_adapt_brightness_checkbox, SIGNAL(toggled(bool)), this, SLOT(set_e2c_adapt_brightness()));
        box_layout->addWidget(_e2c_adapt_brightness_checkbox, row, 1);
        row++;
        QLabel* e2c_isolines_distance_label = new QLabel("Isolines distance:");
        box_layout->addWidget(e2c_isolines_distance_label, row, 0);
        _e2c_isolines_distance_spinbox = new QDoubleSpinBox(this);
        _e2c_isolines_distance_spinbox->setRange(0.0, 10000.00);
        _e2c_isolines_distance_spinbox->setSingleStep(10.0);
        connect(_e2c_isolines_distance_spinbox, SIGNAL(valueChanged(double)), this, SLOT(set_e2c_isolines_distance(double)));
        box_layout->addWidget(_e2c_isolines_distance_spinbox, row, 1);
        row++;
        QLabel* e2c_isolines_thickness_label = new QLabel("Isolines thickness:");
        box_layout->addWidget(e2c_isolines_thickness_label, row, 0);
        _e2c_isolines_thickness_spinbox = new QDoubleSpinBox(this);
        _e2c_isolines_thickness_spinbox->setRange(0.0, 5.00);
        _e2c_isolines_thickness_spinbox->setSingleStep(1.0);
        connect(_e2c_isolines_thickness_spinbox, SIGNAL(valueChanged(double)), this, SLOT(set_e2c_isolines_thickness(double)));
        box_layout->addWidget(_e2c_isolines_thickness_spinbox, row, 1);
        row++;
        QPushButton* e2c_isolines_color_pushbutton = new QPushButton("Isolines color:");
        connect(e2c_isolines_color_pushbutton, SIGNAL(pressed()), this, SLOT(set_e2c_isolines_color()));
        box_layout->addWidget(e2c_isolines_color_pushbutton, row, 0);
        _e2c_isolines_color_label = new QLabel();
        _e2c_isolines_color_label->setScaledContents(true);
        box_layout->addWidget(_e2c_isolines_color_label, row, 1);
        row++;
    } else if (db_category == ecmdb::category_elevation) {
        QLabel* elevation_scale_center_label = new QLabel("Scale center:");
        box_layout->addWidget(elevation_scale_center_label, row, 0);
        _elevation_scale_center_spinbox = new QDoubleSpinBox(this);
        _elevation_scale_center_spinbox->setRange(-10000.0, +10000.0);
        _elevation_scale_center_spinbox->setSingleStep(1.0);
        connect(_elevation_scale_center_spinbox, SIGNAL(valueChanged(double)), this, SLOT(set_elevation_scale_center(double)));
        box_layout->addWidget(_elevation_scale_center_spinbox, row, 1);
        _elevation_scale_center_slider = new QSlider(Qt::Horizontal, this);
        _elevation_scale_center_slider->setRange(-1000, 1000);
        connect(_elevation_scale_center_slider, SIGNAL(valueChanged(int)), this, SLOT(slide_elevation_scale_center(int)));
        box_layout->addWidget(_elevation_scale_center_slider, row, 2, 1, 2);
        row++;
        QLabel* elevation_scale_factor_label = new QLabel("Scale factor:");
        box_layout->addWidget(elevation_scale_factor_label, row, 0);
        _elevation_scale_factor_spinbox = new QDoubleSpinBox(this);
        _elevation_scale_factor_spinbox->setRange(0.0, 100.0);
        _elevation_scale_factor_spinbox->setSingleStep(0.1);
        connect(_elevation_scale_factor_spinbox, SIGNAL(valueChanged(double)), this, SLOT(set_elevation_scale_factor(double)));
        box_layout->addWidget(_elevation_scale_factor_spinbox, row, 1);
        _elevation_scale_factor_slider = new QSlider(Qt::Horizontal, this);
        _elevation_scale_factor_slider->setRange(0, 1000);
        connect(_elevation_scale_factor_slider, SIGNAL(valueChanged(int)), this, SLOT(slide_elevation_scale_factor(int)));
        box_layout->addWidget(_elevation_scale_factor_slider, row, 2, 1, 2);
        row++;
    } else if (db_category == ecmdb::category_texture) {
        QLabel* texture_contrast_label = new QLabel("Contrast:");
        box_layout->addWidget(texture_contrast_label, row, 0);
        _texture_contrast_spinbox = new QDoubleSpinBox(this);
        _texture_contrast_spinbox->setRange(-1.00, +1.00);
        _texture_contrast_spinbox->setSingleStep(0.01);
        connect(_texture_contrast_spinbox, SIGNAL(valueChanged(double)), this, SLOT(set_texture_contrast(double)));
        box_layout->addWidget(_texture_contrast_spinbox, row, 1);
        _texture_contrast_slider = new QSlider(Qt::Horizontal, this);
        _texture_contrast_slider->setRange(-100, 100);
        connect(_texture_contrast_slider, SIGNAL(valueChanged(int)), this, SLOT(slide_texture_contrast(int)));
        box_layout->addWidget(_texture_contrast_slider, row, 2, 1, 2);
        row++;
        QLabel* texture_brightness_label = new QLabel("Brightness:");
        box_layout->addWidget(texture_brightness_label, row, 0);
        _texture_brightness_spinbox = new QDoubleSpinBox(this);
        _texture_brightness_spinbox->setRange(-1.00, +1.00);
        _texture_brightness_spinbox->setSingleStep(0.01);
        connect(_texture_brightness_spinbox, SIGNAL(valueChanged(double)), this, SLOT(set_texture_brightness(double)));
        box_layout->addWidget(_texture_brightness_spinbox, row, 1);
        _texture_brightness_slider = new QSlider(Qt::Horizontal, this);
        _texture_brightness_slider->setRange(-100, 100);
        connect(_texture_brightness_slider, SIGNAL(valueChanged(int)), this, SLOT(slide_texture_brightness(int)));
        box_layout->addWidget(_texture_brightness_slider, row, 2, 1, 2);
        row++;
        QLabel* texture_saturation_label = new QLabel("Saturation:");
        box_layout->addWidget(texture_saturation_label, row, 0);
        _texture_saturation_spinbox = new QDoubleSpinBox(this);
        _texture_saturation_spinbox->setRange(-1.00, +1.00);
        _texture_saturation_spinbox->setSingleStep(0.01);
        connect(_texture_saturation_spinbox, SIGNAL(valueChanged(double)), this, SLOT(set_texture_saturation(double)));
        box_layout->addWidget(_texture_saturation_spinbox, row, 1);
        _texture_saturation_slider = new QSlider(Qt::Horizontal, this);
        _texture_saturation_slider->setRange(-100, 100);
        connect(_texture_saturation_slider, SIGNAL(valueChanged(int)), this, SLOT(slide_texture_saturation(int)));
        box_layout->addWidget(_texture_saturation_slider, row, 2, 1, 2);
        row++;
        QLabel* texture_hue_label = new QLabel("Hue:");
        box_layout->addWidget(texture_hue_label, row, 0);
        _texture_hue_spinbox = new QDoubleSpinBox(this);
        _texture_hue_spinbox->setRange(-1.00, +1.00);
        _texture_hue_spinbox->setSingleStep(0.01);
        connect(_texture_hue_spinbox, SIGNAL(valueChanged(double)), this, SLOT(set_texture_hue(double)));
        box_layout->addWidget(_texture_hue_spinbox, row, 1);
        _texture_hue_slider = new QSlider(Qt::Horizontal, this);
        _texture_hue_slider->setRange(-100, 100);
        connect(_texture_hue_slider, SIGNAL(valueChanged(int)), this, SLOT(slide_texture_hue(int)));
        box_layout->addWidget(_texture_hue_slider, row, 2, 1, 2);
        row++;
    } else if (db_category == ecmdb::category_sar_amplitude) {
        QGroupBox* despeckling_box = new QGroupBox("Despeckling");
        QGridLayout* despeckling_box_layout = new QGridLayout();
        _sar_amplitude_despeckling_method_box = new QComboBox(this);
        despeckling_box_layout->addWidget(_sar_amplitude_despeckling_method_box, 0, 0);
        _sar_amplitude_despeckling_params_stack = new QStackedWidget(this);
        despeckling_box_layout->addWidget(_sar_amplitude_despeckling_params_stack, 1, 0);
        despeckling_box_layout->setRowStretch(2, 1);
        despeckling_box->setLayout(despeckling_box_layout);
        box_layout->addWidget(despeckling_box, row, 0);
        _sar_amplitude_despeckling_method_box->addItem("Off");
        QWidget* despeckling_none_widget = new QWidget(this);
        QGridLayout* despeckling_none_layout = new QGridLayout();
        despeckling_none_widget->setLayout(despeckling_none_layout);
        _sar_amplitude_despeckling_params_stack->addWidget(despeckling_none_widget);
        _sar_amplitude_despeckling_method_box->addItem("Mean");
        QWidget* despeckling_mean_widget = new QWidget(this);
        QGridLayout* despeckling_mean_layout = new QGridLayout();
        despeckling_mean_layout->addWidget(new QLabel("Mask size:"), 0, 0);
        _sar_amplitude_despeckling_mean_masksize_spinbox = new MaskSizeSpinBox(this);
        connect(_sar_amplitude_despeckling_mean_masksize_spinbox, SIGNAL(valueChanged(int)), this, SLOT(set_sar_amplitude_despeckling_mean_masksize(int)));
        despeckling_mean_layout->addWidget(_sar_amplitude_despeckling_mean_masksize_spinbox, 0, 1);
        despeckling_mean_layout->setRowStretch(1, 1);
        despeckling_mean_widget->setLayout(despeckling_mean_layout);
        _sar_amplitude_despeckling_params_stack->addWidget(despeckling_mean_widget);
        _sar_amplitude_despeckling_method_box->addItem("Median");
        QWidget* despeckling_median_widget = new QWidget(this);
        QGridLayout* despeckling_median_layout = new QGridLayout();
        despeckling_median_layout->addWidget(new QLabel("Mask size:"), 0, 0);
        _sar_amplitude_despeckling_median_masksize_spinbox = new MaskSizeSpinBox(this);
        connect(_sar_amplitude_despeckling_median_masksize_spinbox, SIGNAL(valueChanged(int)), this, SLOT(set_sar_amplitude_despeckling_median_masksize(int)));
        despeckling_median_layout->addWidget(_sar_amplitude_despeckling_median_masksize_spinbox, 0, 1);
        despeckling_median_layout->setRowStretch(1, 1);
        despeckling_median_widget->setLayout(despeckling_median_layout);
        _sar_amplitude_despeckling_params_stack->addWidget(despeckling_median_widget);
        _sar_amplitude_despeckling_method_box->addItem("Gauss");
        QWidget* despeckling_gauss_widget = new QWidget(this);
        QGridLayout* despeckling_gauss_layout = new QGridLayout();
        despeckling_gauss_layout->addWidget(new QLabel("Sigma:"), 0, 0);
        _sar_amplitude_despeckling_gauss_sigma_spinbox = new QDoubleSpinBox(this);
        _sar_amplitude_despeckling_gauss_sigma_spinbox->setRange(0.01, 9.99);
        _sar_amplitude_despeckling_gauss_sigma_spinbox->setSingleStep(0.01);
        connect(_sar_amplitude_despeckling_gauss_sigma_spinbox, SIGNAL(valueChanged(double)), this, SLOT(set_sar_amplitude_despeckling_gauss_sigma(double)));
        despeckling_gauss_layout->addWidget(_sar_amplitude_despeckling_gauss_sigma_spinbox, 0, 1);
        despeckling_gauss_layout->setRowStretch(1, 1);
        despeckling_gauss_widget->setLayout(despeckling_gauss_layout);
        _sar_amplitude_despeckling_params_stack->addWidget(despeckling_gauss_widget);
        _sar_amplitude_despeckling_method_box->addItem("Lee");
        QWidget* despeckling_lee_widget = new QWidget(this);
        QGridLayout* despeckling_lee_layout = new QGridLayout();
        despeckling_lee_layout->addWidget(new QLabel("Mask size:"), 0, 0);
        _sar_amplitude_despeckling_lee_masksize_spinbox = new MaskSizeSpinBox(this);
        connect(_sar_amplitude_despeckling_lee_masksize_spinbox, SIGNAL(valueChanged(int)), this, SLOT(set_sar_amplitude_despeckling_lee_masksize(int)));
        despeckling_lee_layout->addWidget(_sar_amplitude_despeckling_lee_masksize_spinbox, 0, 1);
        despeckling_lee_layout->addWidget(new QLabel("Sigma:"), 1, 0);
        _sar_amplitude_despeckling_lee_sigma_spinbox = new QDoubleSpinBox(this);
        _sar_amplitude_despeckling_lee_sigma_spinbox->setRange(0.0, 999.9);
        _sar_amplitude_despeckling_lee_sigma_spinbox->setSingleStep(0.001);
        connect(_sar_amplitude_despeckling_lee_sigma_spinbox, SIGNAL(valueChanged(double)), this, SLOT(set_sar_amplitude_despeckling_lee_sigma(double)));
        despeckling_lee_layout->addWidget(_sar_amplitude_despeckling_lee_sigma_spinbox, 1, 1);
        despeckling_lee_layout->setRowStretch(2, 1);
        despeckling_lee_widget->setLayout(despeckling_lee_layout);
        _sar_amplitude_despeckling_params_stack->addWidget(despeckling_lee_widget);
        _sar_amplitude_despeckling_method_box->addItem("Kuan");
        QWidget* despeckling_kuan_widget = new QWidget(this);
        QGridLayout* despeckling_kuan_layout = new QGridLayout();
        despeckling_kuan_layout->addWidget(new QLabel("Mask size:"), 0, 0);
        _sar_amplitude_despeckling_kuan_masksize_spinbox = new MaskSizeSpinBox(this);
        connect(_sar_amplitude_despeckling_kuan_masksize_spinbox, SIGNAL(valueChanged(int)), this, SLOT(set_sar_amplitude_despeckling_kuan_masksize(int)));
        despeckling_kuan_layout->addWidget(_sar_amplitude_despeckling_kuan_masksize_spinbox, 0, 1);
        despeckling_kuan_layout->addWidget(new QLabel("L:"), 1, 0);
        _sar_amplitude_despeckling_kuan_l_spinbox = new QDoubleSpinBox(this);
        _sar_amplitude_despeckling_kuan_l_spinbox->setRange(0.01, 999.9);
        _sar_amplitude_despeckling_kuan_l_spinbox->setSingleStep(0.001);
        connect(_sar_amplitude_despeckling_kuan_l_spinbox, SIGNAL(valueChanged(double)), this, SLOT(set_sar_amplitude_despeckling_kuan_l(double)));
        despeckling_kuan_layout->addWidget(_sar_amplitude_despeckling_kuan_l_spinbox, 1, 1);
        despeckling_kuan_layout->setRowStretch(2, 1);
        despeckling_kuan_widget->setLayout(despeckling_kuan_layout);
        _sar_amplitude_despeckling_params_stack->addWidget(despeckling_kuan_widget);
        _sar_amplitude_despeckling_method_box->addItem("Xiao");
        QWidget* despeckling_xiao_widget = new QWidget(this);
        QGridLayout* despeckling_xiao_layout = new QGridLayout();
        despeckling_xiao_layout->addWidget(new QLabel("Mask size:"), 0, 0);
        _sar_amplitude_despeckling_xiao_masksize_spinbox = new MaskSizeSpinBox(this);
        connect(_sar_amplitude_despeckling_xiao_masksize_spinbox, SIGNAL(valueChanged(int)), this, SLOT(set_sar_amplitude_despeckling_xiao_masksize(int)));
        despeckling_xiao_layout->addWidget(_sar_amplitude_despeckling_xiao_masksize_spinbox, 0, 1);
        despeckling_xiao_layout->addWidget(new QLabel("T min:"), 1, 0);
        _sar_amplitude_despeckling_xiao_tmin_spinbox = new QDoubleSpinBox(this);
        _sar_amplitude_despeckling_xiao_tmin_spinbox->setRange(-99.9, +99.9);
        _sar_amplitude_despeckling_xiao_tmin_spinbox->setSingleStep(0.1);
        connect(_sar_amplitude_despeckling_xiao_tmin_spinbox, SIGNAL(valueChanged(double)), this, SLOT(set_sar_amplitude_despeckling_xiao_tmin(double)));
        despeckling_xiao_layout->addWidget(_sar_amplitude_despeckling_xiao_tmin_spinbox, 1, 1);
        despeckling_xiao_layout->addWidget(new QLabel("T max:"), 2, 0);
        _sar_amplitude_despeckling_xiao_tmax_spinbox = new QDoubleSpinBox(this);
        _sar_amplitude_despeckling_xiao_tmax_spinbox->setRange(-99.9, +99.9);
        _sar_amplitude_despeckling_xiao_tmax_spinbox->setSingleStep(0.1);
        connect(_sar_amplitude_despeckling_xiao_tmax_spinbox, SIGNAL(valueChanged(double)), this, SLOT(set_sar_amplitude_despeckling_xiao_tmax(double)));
        despeckling_xiao_layout->addWidget(_sar_amplitude_despeckling_xiao_tmax_spinbox, 2, 1);
        despeckling_xiao_layout->addWidget(new QLabel("a:"), 3, 0);
        _sar_amplitude_despeckling_xiao_a_spinbox = new QDoubleSpinBox(this);
        _sar_amplitude_despeckling_xiao_a_spinbox->setRange(0.0, 1.0);
        _sar_amplitude_despeckling_xiao_a_spinbox->setSingleStep(0.01);
        connect(_sar_amplitude_despeckling_xiao_a_spinbox, SIGNAL(valueChanged(double)), this, SLOT(set_sar_amplitude_despeckling_xiao_a(double)));
        despeckling_xiao_layout->addWidget(_sar_amplitude_despeckling_xiao_a_spinbox, 3, 1);
        despeckling_xiao_layout->addWidget(new QLabel("b:"), 4, 0);
        _sar_amplitude_despeckling_xiao_b_spinbox = new QDoubleSpinBox(this);
        _sar_amplitude_despeckling_xiao_b_spinbox->setRange(0.0, 1.0);
        _sar_amplitude_despeckling_xiao_b_spinbox->setSingleStep(0.01);
        connect(_sar_amplitude_despeckling_xiao_b_spinbox, SIGNAL(valueChanged(double)), this, SLOT(set_sar_amplitude_despeckling_xiao_b(double)));
        despeckling_xiao_layout->addWidget(_sar_amplitude_despeckling_xiao_b_spinbox, 4, 1);
        despeckling_xiao_layout->setRowStretch(5, 1);
        despeckling_xiao_widget->setLayout(despeckling_xiao_layout);
        _sar_amplitude_despeckling_params_stack->addWidget(despeckling_xiao_widget);
        _sar_amplitude_despeckling_method_box->addItem("Frost");
        QWidget* despeckling_frost_widget = new QWidget(this);
        QGridLayout* despeckling_frost_layout = new QGridLayout();
        despeckling_frost_layout->addWidget(new QLabel("Mask size:"), 0, 0);
        _sar_amplitude_despeckling_frost_masksize_spinbox = new MaskSizeSpinBox(this);
        connect(_sar_amplitude_despeckling_frost_masksize_spinbox, SIGNAL(valueChanged(int)), this, SLOT(set_sar_amplitude_despeckling_frost_masksize(int)));
        despeckling_frost_layout->addWidget(_sar_amplitude_despeckling_frost_masksize_spinbox, 0, 1);
        despeckling_frost_layout->addWidget(new QLabel("a:"), 1, 0);
        _sar_amplitude_despeckling_frost_a_spinbox = new QDoubleSpinBox(this);
        _sar_amplitude_despeckling_frost_a_spinbox->setRange(-99.9, +99.9);
        _sar_amplitude_despeckling_frost_a_spinbox->setSingleStep(0.01);
        connect(_sar_amplitude_despeckling_frost_a_spinbox, SIGNAL(valueChanged(double)), this, SLOT(set_sar_amplitude_despeckling_frost_a(double)));
        despeckling_frost_layout->addWidget(_sar_amplitude_despeckling_frost_a_spinbox, 1, 1);
        despeckling_frost_layout->setRowStretch(2, 1);
        despeckling_frost_widget->setLayout(despeckling_frost_layout);
        _sar_amplitude_despeckling_params_stack->addWidget(despeckling_frost_widget);
        _sar_amplitude_despeckling_method_box->addItem("Gamma MAP");
        QWidget* despeckling_gammamap_widget = new QWidget(this);
        QGridLayout* despeckling_gammamap_layout = new QGridLayout();
        despeckling_gammamap_layout->addWidget(new QLabel("Mask size:"), 0, 0);
        _sar_amplitude_despeckling_gammamap_masksize_spinbox = new MaskSizeSpinBox(this);
        connect(_sar_amplitude_despeckling_gammamap_masksize_spinbox, SIGNAL(valueChanged(int)), this, SLOT(set_sar_amplitude_despeckling_gammamap_masksize(int)));
        despeckling_gammamap_layout->addWidget(_sar_amplitude_despeckling_gammamap_masksize_spinbox, 0, 1);
        despeckling_gammamap_layout->addWidget(new QLabel("L:"), 1, 0);
        _sar_amplitude_despeckling_gammamap_l_spinbox = new QDoubleSpinBox(this);
        _sar_amplitude_despeckling_gammamap_l_spinbox->setRange(0.01, 99.9);
        connect(_sar_amplitude_despeckling_gammamap_l_spinbox, SIGNAL(valueChanged(double)), this, SLOT(set_sar_amplitude_despeckling_gammamap_l(double)));
        despeckling_gammamap_layout->addWidget(_sar_amplitude_despeckling_gammamap_l_spinbox, 1, 1);
        despeckling_gammamap_layout->setRowStretch(2, 1);
        despeckling_gammamap_widget->setLayout(despeckling_gammamap_layout);
        _sar_amplitude_despeckling_params_stack->addWidget(despeckling_gammamap_widget);
        _sar_amplitude_despeckling_method_box->addItem("Oddy");
        QWidget* despeckling_oddy_widget = new QWidget(this);
        QGridLayout* despeckling_oddy_layout = new QGridLayout();
        despeckling_oddy_layout->addWidget(new QLabel("Mask size:"), 0, 0);
        _sar_amplitude_despeckling_oddy_masksize_spinbox = new MaskSizeSpinBox(this);
        connect(_sar_amplitude_despeckling_oddy_masksize_spinbox, SIGNAL(valueChanged(int)), this, SLOT(set_sar_amplitude_despeckling_oddy_masksize(int)));
        despeckling_oddy_layout->addWidget(_sar_amplitude_despeckling_oddy_masksize_spinbox, 0, 1);
        despeckling_oddy_layout->addWidget(new QLabel("alpha:"), 1, 0);
        _sar_amplitude_despeckling_oddy_alpha_spinbox = new QDoubleSpinBox(this);
        _sar_amplitude_despeckling_oddy_alpha_spinbox->setRange(0.0, 99.9);
        _sar_amplitude_despeckling_oddy_alpha_spinbox->setSingleStep(0.1);
        connect(_sar_amplitude_despeckling_oddy_alpha_spinbox, SIGNAL(valueChanged(double)), this, SLOT(set_sar_amplitude_despeckling_oddy_alpha(double)));
        despeckling_oddy_layout->addWidget(_sar_amplitude_despeckling_oddy_alpha_spinbox, 1, 1);
        despeckling_oddy_layout->setRowStretch(2, 1);
        despeckling_oddy_widget->setLayout(despeckling_oddy_layout);
        _sar_amplitude_despeckling_params_stack->addWidget(despeckling_oddy_widget);
        _sar_amplitude_despeckling_method_box->addItem("Wavelet ST");
        QWidget* despeckling_waveletst_widget = new QWidget(this);
        QGridLayout* despeckling_waveletst_layout = new QGridLayout();
        despeckling_waveletst_layout->addWidget(new QLabel("Threshold:"), 0, 0);
        _sar_amplitude_despeckling_waveletst_threshold_spinbox = new QDoubleSpinBox(this);
        _sar_amplitude_despeckling_waveletst_threshold_spinbox->setRange(0.0, 1.0);
        _sar_amplitude_despeckling_waveletst_threshold_spinbox->setSingleStep(0.0001);
        _sar_amplitude_despeckling_waveletst_threshold_spinbox->setDecimals(4);
        connect(_sar_amplitude_despeckling_waveletst_threshold_spinbox, SIGNAL(valueChanged(double)), this, SLOT(set_sar_amplitude_despeckling_waveletst_threshold(double)));
        despeckling_waveletst_layout->addWidget(_sar_amplitude_despeckling_waveletst_threshold_spinbox, 0, 1);
        despeckling_waveletst_layout->setRowStretch(2, 1);
        despeckling_waveletst_widget->setLayout(despeckling_waveletst_layout);
        _sar_amplitude_despeckling_params_stack->addWidget(despeckling_waveletst_widget);
        QGroupBox* drr_box = new QGroupBox("Dyn. Range Reduction");
        QGridLayout* drr_box_layout = new QGridLayout();
        _sar_amplitude_drr_method_box = new QComboBox(this);
        drr_box_layout->addWidget(_sar_amplitude_drr_method_box, 0, 0);
        _sar_amplitude_drr_params_stack = new QStackedWidget(this);
        drr_box_layout->addWidget(_sar_amplitude_drr_params_stack, 1, 0);
        drr_box_layout->setRowStretch(2, 1);
        drr_box->setLayout(drr_box_layout);
        box_layout->addWidget(drr_box, row, 1);
        _sar_amplitude_drr_method_box->addItem("Linear");
        QWidget* drr_linear_widget = new QWidget(this);
        QGridLayout* drr_linear_layout = new QGridLayout();
        drr_linear_layout->addWidget(new QLabel("Min. amplitude:"), 0, 0);
        _sar_amplitude_drr_linear_minamp_spinbox = new QDoubleSpinBox(this);
        _sar_amplitude_drr_linear_minamp_spinbox->setRange(0.0, 1.0);
        _sar_amplitude_drr_linear_minamp_spinbox->setSingleStep(0.001);
        connect(_sar_amplitude_drr_linear_minamp_spinbox, SIGNAL(valueChanged(double)), this, SLOT(set_sar_amplitude_drr_linear_minamp(double)));
        drr_linear_layout->addWidget(_sar_amplitude_drr_linear_minamp_spinbox, 0, 1);
        _sar_amplitude_drr_linear_minamp_slider = new QSlider(Qt::Horizontal, this);
        _sar_amplitude_drr_linear_minamp_slider->setRange(0, 1000);
        connect(_sar_amplitude_drr_linear_minamp_slider, SIGNAL(valueChanged(int)), this, SLOT(slide_sar_amplitude_drr_linear_minamp(int)));
        drr_linear_layout->addWidget(_sar_amplitude_drr_linear_minamp_slider, 1, 0, 1, 2);
        drr_linear_layout->addWidget(new QLabel("Max. amplitude:"), 2, 0);
        _sar_amplitude_drr_linear_maxamp_spinbox = new QDoubleSpinBox(this);
        _sar_amplitude_drr_linear_maxamp_spinbox->setRange(0.0, 1.0);
        _sar_amplitude_drr_linear_maxamp_spinbox->setSingleStep(0.001);
        connect(_sar_amplitude_drr_linear_maxamp_spinbox, SIGNAL(valueChanged(double)), this, SLOT(set_sar_amplitude_drr_linear_maxamp(double)));
        drr_linear_layout->addWidget(_sar_amplitude_drr_linear_maxamp_spinbox, 2, 1);
        _sar_amplitude_drr_linear_maxamp_slider = new QSlider(Qt::Horizontal, this);
        _sar_amplitude_drr_linear_maxamp_slider->setRange(0, 1000);
        connect(_sar_amplitude_drr_linear_maxamp_slider, SIGNAL(valueChanged(int)), this, SLOT(slide_sar_amplitude_drr_linear_maxamp(int)));
        drr_linear_layout->addWidget(_sar_amplitude_drr_linear_maxamp_slider, 3, 0, 1, 2);
        drr_linear_layout->setRowStretch(4, 1);
        drr_linear_widget->setLayout(drr_linear_layout);
        _sar_amplitude_drr_params_stack->addWidget(drr_linear_widget);
        _sar_amplitude_drr_method_box->addItem("Logarithmic");
        QWidget* drr_log_widget = new QWidget(this);
        QGridLayout* drr_log_layout = new QGridLayout();
        drr_log_layout->addWidget(new QLabel("Min. amplitude:"), 0, 0);
        _sar_amplitude_drr_log_minamp_spinbox = new QDoubleSpinBox(this);
        _sar_amplitude_drr_log_minamp_spinbox->setRange(0.0, 1.0);
        _sar_amplitude_drr_log_minamp_spinbox->setSingleStep(0.001);
        connect(_sar_amplitude_drr_log_minamp_spinbox, SIGNAL(valueChanged(double)), this, SLOT(set_sar_amplitude_drr_log_minamp(double)));
        drr_log_layout->addWidget(_sar_amplitude_drr_log_minamp_spinbox, 0, 1);
        _sar_amplitude_drr_log_minamp_slider = new QSlider(Qt::Horizontal, this);
        _sar_amplitude_drr_log_minamp_slider->setRange(0, 1000);
        connect(_sar_amplitude_drr_log_minamp_slider, SIGNAL(valueChanged(int)), this, SLOT(slide_sar_amplitude_drr_log_minamp(int)));
        drr_log_layout->addWidget(_sar_amplitude_drr_log_minamp_slider, 1, 0, 1, 2);
        drr_log_layout->addWidget(new QLabel("Max. amplitude:"), 2, 0);
        _sar_amplitude_drr_log_maxamp_spinbox = new QDoubleSpinBox(this);
        _sar_amplitude_drr_log_maxamp_spinbox->setRange(0.0, 1.0);
        _sar_amplitude_drr_log_maxamp_spinbox->setSingleStep(0.001);
        connect(_sar_amplitude_drr_log_maxamp_spinbox, SIGNAL(valueChanged(double)), this, SLOT(set_sar_amplitude_drr_log_maxamp(double)));
        drr_log_layout->addWidget(_sar_amplitude_drr_log_maxamp_spinbox, 2, 1);
        _sar_amplitude_drr_log_maxamp_slider = new QSlider(Qt::Horizontal, this);
        _sar_amplitude_drr_log_maxamp_slider->setRange(0, 1000);
        connect(_sar_amplitude_drr_log_maxamp_slider, SIGNAL(valueChanged(int)), this, SLOT(slide_sar_amplitude_drr_log_maxamp(int)));
        drr_log_layout->addWidget(_sar_amplitude_drr_log_maxamp_slider, 3, 0, 1, 2);
        drr_log_layout->addWidget(new QLabel("Prescale:"), 4, 0);
        _sar_amplitude_drr_log_prescale_spinbox = new QDoubleSpinBox(this);
        _sar_amplitude_drr_log_prescale_spinbox->setRange(1.0, 10000.0);
        _sar_amplitude_drr_log_prescale_spinbox->setSingleStep(1.0);
        connect(_sar_amplitude_drr_log_prescale_spinbox, SIGNAL(valueChanged(double)), this, SLOT(set_sar_amplitude_drr_log_prescale(double)));
        drr_log_layout->addWidget(_sar_amplitude_drr_log_prescale_spinbox, 4, 1);
        _sar_amplitude_drr_log_prescale_slider = new QSlider(Qt::Horizontal, this);
        _sar_amplitude_drr_log_prescale_slider->setRange(1, 10000);
        connect(_sar_amplitude_drr_log_prescale_slider, SIGNAL(valueChanged(int)), this, SLOT(slide_sar_amplitude_drr_log_prescale(int)));
        drr_log_layout->addWidget(_sar_amplitude_drr_log_prescale_slider, 5, 0, 1, 2);
        drr_log_layout->setRowStretch(6, 1);
        drr_log_widget->setLayout(drr_log_layout);
        _sar_amplitude_drr_params_stack->addWidget(drr_log_widget);
        _sar_amplitude_drr_method_box->addItem("Gamma");
        QWidget* drr_gamma_widget = new QWidget(this);
        QGridLayout* drr_gamma_layout = new QGridLayout();
        drr_gamma_layout->addWidget(new QLabel("Min. amplitude:"), 0, 0);
        _sar_amplitude_drr_gamma_minamp_spinbox = new QDoubleSpinBox(this);
        _sar_amplitude_drr_gamma_minamp_spinbox->setRange(0.0, 1.0);
        _sar_amplitude_drr_gamma_minamp_spinbox->setSingleStep(0.001);
        connect(_sar_amplitude_drr_gamma_minamp_spinbox, SIGNAL(valueChanged(double)), this, SLOT(set_sar_amplitude_drr_gamma_minamp(double)));
        drr_gamma_layout->addWidget(_sar_amplitude_drr_gamma_minamp_spinbox, 0, 1);
        _sar_amplitude_drr_gamma_minamp_slider = new QSlider(Qt::Horizontal, this);
        _sar_amplitude_drr_gamma_minamp_slider->setRange(0, 1000);
        connect(_sar_amplitude_drr_gamma_minamp_slider, SIGNAL(valueChanged(int)), this, SLOT(slide_sar_amplitude_drr_gamma_minamp(int)));
        drr_gamma_layout->addWidget(_sar_amplitude_drr_gamma_minamp_slider, 1, 0, 1, 2);
        drr_gamma_layout->addWidget(new QLabel("Max. amplitude:"), 2, 0);
        _sar_amplitude_drr_gamma_maxamp_spinbox = new QDoubleSpinBox(this);
        _sar_amplitude_drr_gamma_maxamp_spinbox->setRange(0.0, 1.0);
        _sar_amplitude_drr_gamma_maxamp_spinbox->setSingleStep(0.001);
        connect(_sar_amplitude_drr_gamma_maxamp_spinbox, SIGNAL(valueChanged(double)), this, SLOT(set_sar_amplitude_drr_gamma_maxamp(double)));
        drr_gamma_layout->addWidget(_sar_amplitude_drr_gamma_maxamp_spinbox, 2, 1);
        _sar_amplitude_drr_gamma_maxamp_slider = new QSlider(Qt::Horizontal, this);
        _sar_amplitude_drr_gamma_maxamp_slider->setRange(0, 1000);
        connect(_sar_amplitude_drr_gamma_maxamp_slider, SIGNAL(valueChanged(int)), this, SLOT(slide_sar_amplitude_drr_gamma_maxamp(int)));
        drr_gamma_layout->addWidget(_sar_amplitude_drr_gamma_maxamp_slider, 3, 0, 1, 2);
        drr_gamma_layout->addWidget(new QLabel("Gamma:"), 4, 0);
        _sar_amplitude_drr_gamma_gamma_spinbox = new QDoubleSpinBox(this);
        _sar_amplitude_drr_gamma_gamma_spinbox->setRange(0.25, 4.0);
        _sar_amplitude_drr_gamma_gamma_spinbox->setSingleStep(0.01);
        connect(_sar_amplitude_drr_gamma_gamma_spinbox, SIGNAL(valueChanged(double)), this, SLOT(set_sar_amplitude_drr_gamma_gamma(double)));
        drr_gamma_layout->addWidget(_sar_amplitude_drr_gamma_gamma_spinbox, 4, 1);
        _sar_amplitude_drr_gamma_gamma_slider = new QSlider(Qt::Horizontal, this);
        _sar_amplitude_drr_gamma_gamma_slider->setRange(-300, 300);
        connect(_sar_amplitude_drr_gamma_gamma_slider, SIGNAL(valueChanged(int)), this, SLOT(slide_sar_amplitude_drr_gamma_gamma(int)));
        drr_gamma_layout->addWidget(_sar_amplitude_drr_gamma_gamma_slider, 5, 0, 1, 2);
        drr_gamma_layout->setRowStretch(6, 1);
        drr_gamma_widget->setLayout(drr_gamma_layout);
        _sar_amplitude_drr_params_stack->addWidget(drr_gamma_widget);
        _sar_amplitude_drr_method_box->addItem("Schlick");
        QWidget* drr_schlick_widget = new QWidget(this);
        QGridLayout* drr_schlick_layout = new QGridLayout();
        drr_schlick_layout->addWidget(new QLabel("Brightness:"), 0, 0);
        _sar_amplitude_drr_schlick_brightness_spinbox = new QDoubleSpinBox(this);
        _sar_amplitude_drr_schlick_brightness_spinbox->setRange(1.0, 500.0);
        _sar_amplitude_drr_schlick_brightness_spinbox->setSingleStep(1.0);
        connect(_sar_amplitude_drr_schlick_brightness_spinbox, SIGNAL(valueChanged(double)), this, SLOT(set_sar_amplitude_drr_schlick_brightness(double)));
        drr_schlick_layout->addWidget(_sar_amplitude_drr_schlick_brightness_spinbox, 0, 1);
        _sar_amplitude_drr_schlick_brightness_slider = new QSlider(Qt::Horizontal, this);
        _sar_amplitude_drr_schlick_brightness_slider->setRange(1, 500);
        connect(_sar_amplitude_drr_schlick_brightness_slider, SIGNAL(valueChanged(int)), this, SLOT(slide_sar_amplitude_drr_schlick_brightness(int)));
        drr_schlick_layout->addWidget(_sar_amplitude_drr_schlick_brightness_slider, 1, 0, 1, 2);
        drr_schlick_layout->setRowStretch(2, 1);
        drr_schlick_widget->setLayout(drr_schlick_layout);
        _sar_amplitude_drr_params_stack->addWidget(drr_schlick_widget);
        _sar_amplitude_drr_method_box->addItem("Reinhard");
        QWidget* drr_reinhard_widget = new QWidget(this);
        QGridLayout* drr_reinhard_layout = new QGridLayout();
        drr_reinhard_layout->addWidget(new QLabel("Brightness:"), 0, 0);
        _sar_amplitude_drr_reinhard_brightness_spinbox = new QDoubleSpinBox(this);
        _sar_amplitude_drr_reinhard_brightness_spinbox->setRange(-8.0, +8.0);
        _sar_amplitude_drr_reinhard_brightness_spinbox->setSingleStep(0.1);
        connect(_sar_amplitude_drr_reinhard_brightness_spinbox, SIGNAL(valueChanged(double)), this, SLOT(set_sar_amplitude_drr_reinhard_brightness(double)));
        drr_reinhard_layout->addWidget(_sar_amplitude_drr_reinhard_brightness_spinbox, 0, 1);
        _sar_amplitude_drr_reinhard_brightness_slider = new QSlider(Qt::Horizontal, this);
        _sar_amplitude_drr_reinhard_brightness_slider->setRange(-800, +800);
        connect(_sar_amplitude_drr_reinhard_brightness_slider, SIGNAL(valueChanged(int)), this, SLOT(slide_sar_amplitude_drr_reinhard_brightness(int)));
        drr_reinhard_layout->addWidget(_sar_amplitude_drr_reinhard_brightness_slider, 1, 0, 1, 2);
        drr_reinhard_layout->addWidget(new QLabel("Contrast:"), 2, 0);
        _sar_amplitude_drr_reinhard_contrast_spinbox = new QDoubleSpinBox(this);
        _sar_amplitude_drr_reinhard_contrast_spinbox->setRange(0.0, 1.0);
        _sar_amplitude_drr_reinhard_contrast_spinbox->setSingleStep(0.01);
        connect(_sar_amplitude_drr_reinhard_contrast_spinbox, SIGNAL(valueChanged(double)), this, SLOT(set_sar_amplitude_drr_reinhard_contrast(double)));
        drr_reinhard_layout->addWidget(_sar_amplitude_drr_reinhard_contrast_spinbox, 2, 1);
        _sar_amplitude_drr_reinhard_contrast_slider = new QSlider(Qt::Horizontal, this);
        _sar_amplitude_drr_reinhard_contrast_slider->setRange(0, 1000);
        connect(_sar_amplitude_drr_reinhard_contrast_slider, SIGNAL(valueChanged(int)), this, SLOT(slide_sar_amplitude_drr_reinhard_contrast(int)));
        drr_reinhard_layout->addWidget(_sar_amplitude_drr_reinhard_contrast_slider, 3, 0, 1, 2);
        drr_reinhard_layout->setRowStretch(4, 1);
        drr_reinhard_widget->setLayout(drr_reinhard_layout);
        _sar_amplitude_drr_params_stack->addWidget(drr_reinhard_widget);
        _sar_amplitude_drr_method_box->addItem("Schlick local");
        QWidget* drr_schlicklocal_widget = new QWidget(this);
        QGridLayout* drr_schlicklocal_layout = new QGridLayout();
        drr_schlicklocal_layout->addWidget(new QLabel("Brightness:"), 0, 0);
        _sar_amplitude_drr_schlicklocal_brightness_spinbox = new QDoubleSpinBox(this);
        _sar_amplitude_drr_schlicklocal_brightness_spinbox->setRange(-8.00, +8.00);
        _sar_amplitude_drr_schlicklocal_brightness_spinbox->setSingleStep(0.1);
        connect(_sar_amplitude_drr_schlicklocal_brightness_spinbox, SIGNAL(valueChanged(double)), this, SLOT(set_sar_amplitude_drr_schlicklocal_brightness(double)));
        drr_schlicklocal_layout->addWidget(_sar_amplitude_drr_schlicklocal_brightness_spinbox, 0, 1);
        _sar_amplitude_drr_schlicklocal_brightness_slider = new QSlider(Qt::Horizontal, this);
        _sar_amplitude_drr_schlicklocal_brightness_slider->setRange(-800, +800);
        connect(_sar_amplitude_drr_schlicklocal_brightness_slider, SIGNAL(valueChanged(int)), this, SLOT(slide_sar_amplitude_drr_schlicklocal_brightness(int)));
        drr_schlicklocal_layout->addWidget(_sar_amplitude_drr_schlicklocal_brightness_slider, 1, 0, 1, 2);
        drr_schlicklocal_layout->addWidget(new QLabel("Detail accentuation:"), 2, 0);
        _sar_amplitude_drr_schlicklocal_details_spinbox = new QDoubleSpinBox(this);
        _sar_amplitude_drr_schlicklocal_details_spinbox->setRange(0.0, 1.0);
        _sar_amplitude_drr_schlicklocal_details_spinbox->setSingleStep(0.01);
        connect(_sar_amplitude_drr_schlicklocal_details_spinbox, SIGNAL(valueChanged(double)), this, SLOT(set_sar_amplitude_drr_schlicklocal_details(double)));
        drr_schlicklocal_layout->addWidget(_sar_amplitude_drr_schlicklocal_details_spinbox, 2, 1);
        _sar_amplitude_drr_schlicklocal_details_slider = new QSlider(Qt::Horizontal, this);
        _sar_amplitude_drr_schlicklocal_details_slider->setRange(0, 1000);
        connect(_sar_amplitude_drr_schlicklocal_details_slider, SIGNAL(valueChanged(int)), this, SLOT(slide_sar_amplitude_drr_schlicklocal_details(int)));
        drr_schlicklocal_layout->addWidget(_sar_amplitude_drr_schlicklocal_details_slider, 3, 0, 1, 2);
        drr_schlicklocal_layout->addWidget(new QLabel("Threshold:"), 4, 0);
        _sar_amplitude_drr_schlicklocal_threshold_spinbox = new QDoubleSpinBox(this);
        _sar_amplitude_drr_schlicklocal_threshold_spinbox->setRange(-1.0, +1.0);
        _sar_amplitude_drr_schlicklocal_threshold_spinbox->setSingleStep(0.01);
        connect(_sar_amplitude_drr_schlicklocal_threshold_spinbox, SIGNAL(valueChanged(double)), this, SLOT(set_sar_amplitude_drr_schlicklocal_threshold(double)));
        drr_schlicklocal_layout->addWidget(_sar_amplitude_drr_schlicklocal_threshold_spinbox, 4, 1);
        _sar_amplitude_drr_schlicklocal_threshold_slider = new QSlider(Qt::Horizontal, this);
        _sar_amplitude_drr_schlicklocal_threshold_slider->setRange(-1000, +1000);
        connect(_sar_amplitude_drr_schlicklocal_threshold_slider, SIGNAL(valueChanged(int)), this, SLOT(slide_sar_amplitude_drr_schlicklocal_threshold(int)));
        drr_schlicklocal_layout->addWidget(_sar_amplitude_drr_schlicklocal_threshold_slider, 5, 0, 1, 2);
        drr_schlicklocal_layout->setRowStretch(6, 1);
        drr_schlicklocal_widget->setLayout(drr_schlicklocal_layout);
        _sar_amplitude_drr_params_stack->addWidget(drr_schlicklocal_widget);
        _sar_amplitude_drr_method_box->addItem("Reinhard local");
        QWidget* drr_reinhardlocal_widget = new QWidget(this);
        QGridLayout* drr_reinhardlocal_layout = new QGridLayout();
        drr_reinhardlocal_layout->addWidget(new QLabel("Brightness:"), 0, 0);
        _sar_amplitude_drr_reinhardlocal_brightness_spinbox = new QDoubleSpinBox(this);
        _sar_amplitude_drr_reinhardlocal_brightness_spinbox->setRange(-8.00, +8.00);
        _sar_amplitude_drr_reinhardlocal_brightness_spinbox->setSingleStep(0.1);
        connect(_sar_amplitude_drr_reinhardlocal_brightness_spinbox, SIGNAL(valueChanged(double)), this, SLOT(set_sar_amplitude_drr_reinhardlocal_brightness(double)));
        drr_reinhardlocal_layout->addWidget(_sar_amplitude_drr_reinhardlocal_brightness_spinbox, 0, 1);
        _sar_amplitude_drr_reinhardlocal_brightness_slider = new QSlider(Qt::Horizontal, this);
        _sar_amplitude_drr_reinhardlocal_brightness_slider->setRange(-800, +800);
        connect(_sar_amplitude_drr_reinhardlocal_brightness_slider, SIGNAL(valueChanged(int)), this, SLOT(slide_sar_amplitude_drr_reinhardlocal_brightness(int)));
        drr_reinhardlocal_layout->addWidget(_sar_amplitude_drr_reinhardlocal_brightness_slider, 1, 0, 1, 2);
        drr_reinhardlocal_layout->addWidget(new QLabel("Contrast:"), 2, 0);
        _sar_amplitude_drr_reinhardlocal_contrast_spinbox = new QDoubleSpinBox(this);
        _sar_amplitude_drr_reinhardlocal_contrast_spinbox->setRange(0.0, 1.0);
        _sar_amplitude_drr_reinhardlocal_contrast_spinbox->setSingleStep(0.01);
        connect(_sar_amplitude_drr_reinhardlocal_contrast_spinbox, SIGNAL(valueChanged(double)), this, SLOT(set_sar_amplitude_drr_reinhardlocal_contrast(double)));
        drr_reinhardlocal_layout->addWidget(_sar_amplitude_drr_reinhardlocal_contrast_spinbox, 2, 1);
        _sar_amplitude_drr_reinhardlocal_contrast_slider = new QSlider(Qt::Horizontal, this);
        _sar_amplitude_drr_reinhardlocal_contrast_slider->setRange(0, 1000);
        connect(_sar_amplitude_drr_reinhardlocal_contrast_slider, SIGNAL(valueChanged(int)), this, SLOT(slide_sar_amplitude_drr_reinhardlocal_contrast(int)));
        drr_reinhardlocal_layout->addWidget(_sar_amplitude_drr_reinhardlocal_contrast_slider, 3, 0, 1, 2);
        drr_reinhardlocal_layout->addWidget(new QLabel("Detail accentuation:"), 4, 0);
        _sar_amplitude_drr_reinhardlocal_details_spinbox = new QDoubleSpinBox(this);
        _sar_amplitude_drr_reinhardlocal_details_spinbox->setRange(0.0, 1.0);
        _sar_amplitude_drr_reinhardlocal_details_spinbox->setSingleStep(0.01);
        connect(_sar_amplitude_drr_reinhardlocal_details_spinbox, SIGNAL(valueChanged(double)), this, SLOT(set_sar_amplitude_drr_reinhardlocal_details(double)));
        drr_reinhardlocal_layout->addWidget(_sar_amplitude_drr_reinhardlocal_details_spinbox, 4, 1);
        _sar_amplitude_drr_reinhardlocal_details_slider = new QSlider(Qt::Horizontal, this);
        _sar_amplitude_drr_reinhardlocal_details_slider->setRange(0, 1000);
        connect(_sar_amplitude_drr_reinhardlocal_details_slider, SIGNAL(valueChanged(int)), this, SLOT(slide_sar_amplitude_drr_reinhardlocal_details(int)));
        drr_reinhardlocal_layout->addWidget(_sar_amplitude_drr_reinhardlocal_details_slider, 5, 0, 1, 2);
        drr_reinhardlocal_layout->addWidget(new QLabel("Threshold:"), 6, 0);
        _sar_amplitude_drr_reinhardlocal_threshold_spinbox = new QDoubleSpinBox(this);
        _sar_amplitude_drr_reinhardlocal_threshold_spinbox->setRange(-1.0, +1.0);
        _sar_amplitude_drr_reinhardlocal_threshold_spinbox->setSingleStep(0.01);
        connect(_sar_amplitude_drr_reinhardlocal_threshold_spinbox, SIGNAL(valueChanged(double)), this, SLOT(set_sar_amplitude_drr_reinhardlocal_threshold(double)));
        drr_reinhardlocal_layout->addWidget(_sar_amplitude_drr_reinhardlocal_threshold_spinbox, 6, 1);
        _sar_amplitude_drr_reinhardlocal_threshold_slider = new QSlider(Qt::Horizontal, this);
        _sar_amplitude_drr_reinhardlocal_threshold_slider->setRange(-1000, +1000);
        connect(_sar_amplitude_drr_reinhardlocal_threshold_slider, SIGNAL(valueChanged(int)), this, SLOT(slide_sar_amplitude_drr_reinhardlocal_threshold(int)));
        drr_reinhardlocal_layout->addWidget(_sar_amplitude_drr_reinhardlocal_threshold_slider, 7, 0, 1, 2);
        drr_reinhardlocal_layout->setRowStretch(8, 1);
        drr_reinhardlocal_widget->setLayout(drr_reinhardlocal_layout);
        _sar_amplitude_drr_params_stack->addWidget(drr_reinhardlocal_widget);
        connect(_sar_amplitude_despeckling_method_box, SIGNAL(activated(int)), this, SLOT(set_sar_amplitude_despeckling_method(int)));
        connect(_sar_amplitude_drr_method_box, SIGNAL(activated(int)), this, SLOT(set_sar_amplitude_drr_method(int)));
        row++;
        QPushButton* sar_amplitude_gradient_pushbutton = new QPushButton("Color gradient:");
        connect(sar_amplitude_gradient_pushbutton, SIGNAL(pressed()), this, SLOT(set_sar_amplitude_gradient()));
        box_layout->addWidget(sar_amplitude_gradient_pushbutton, row, 0);
        _sar_amplitude_gradient_label = new QLabel();
        _sar_amplitude_gradient_label->setScaledContents(true);
        box_layout->addWidget(_sar_amplitude_gradient_label, row, 1);
        row++;
        QLabel* sar_amplitude_adapt_brightness_label = new QLabel("Adapt brightness:");
        box_layout->addWidget(sar_amplitude_adapt_brightness_label, row, 0);
        _sar_amplitude_adapt_brightness_checkbox = new QCheckBox(this);
        connect(_sar_amplitude_adapt_brightness_checkbox, SIGNAL(toggled(bool)), this, SLOT(set_sar_amplitude_adapt_brightness()));
        box_layout->addWidget(_sar_amplitude_adapt_brightness_checkbox, row, 1);
        row++;
    } else if (db_category == ecmdb::category_data) {
        QLabel* data_offset_label = new QLabel("Offset:");
        box_layout->addWidget(data_offset_label, row, 0);
        _data_offset_spinbox = new QDoubleSpinBox(this);
        _data_offset_spinbox->setRange(0.0, 10.0);
        _data_offset_spinbox->setSingleStep(0.1);
        connect(_data_offset_spinbox, SIGNAL(valueChanged(double)), this, SLOT(set_data_offset(double)));
        box_layout->addWidget(_data_offset_spinbox, row, 1);
        _data_offset_slider = new QSlider(Qt::Horizontal, this);
        _data_offset_slider->setRange(0, 100);
        connect(_data_offset_slider, SIGNAL(valueChanged(int)), this, SLOT(slide_data_offset(int)));
        box_layout->addWidget(_data_offset_slider, 0, 2, 1, 2);
        row++;
        QLabel* data_factor_label = new QLabel("Factor:");
        box_layout->addWidget(data_factor_label, row, 0);
        _data_factor_spinbox = new QDoubleSpinBox(this);
        _data_factor_spinbox->setRange(0.0, 10.0);
        _data_factor_spinbox->setSingleStep(0.1);
        connect(_data_factor_spinbox, SIGNAL(valueChanged(double)), this, SLOT(set_data_factor(double)));
        box_layout->addWidget(_data_factor_spinbox, row, 1);
        _data_factor_slider = new QSlider(Qt::Horizontal, this);
        _data_factor_slider->setRange(0, 100);
        connect(_data_factor_slider, SIGNAL(valueChanged(int)), this, SLOT(slide_data_factor(int)));
        box_layout->addWidget(_data_factor_slider, 0, 2, 1, 2);
        row++;
        QPushButton* data_gradient_pushbutton = new QPushButton("Color gradient:");
        connect(data_gradient_pushbutton, SIGNAL(pressed()), this, SLOT(set_data_gradient()));
        box_layout->addWidget(data_gradient_pushbutton, row, 0, 1, 2);
        _data_gradient_label = new QLabel();
        _data_gradient_label->setScaledContents(true);
        box_layout->addWidget(_data_gradient_label, row, 2, 1, 2);
        row++;
    }
    box_layout->setRowStretch(row, 1);
    box->setLayout(box_layout);

    setLayout(layout);
    update();
}

QPixmap DBProcessingParameters::pixmap_from_gradient(int gradient_length, const uint8_t* gradient)
{
    QImage img(gradient_length, 1, QImage::Format_RGB888);
    for (int i = 0; i < gradient_length; i++) {
        img.setPixel(i, 0, qRgb(gradient[3 * i + 0], gradient[3 * i + 1], gradient[3 * i + 2]));
    }
    return QPixmap::fromImage(img);
}

void DBProcessingParameters::choose_gradient(int& gradient_length, uint8_t* gradient)
{
    QDialog* dlg = new QDialog(this);
    dlg->setWindowTitle("Choose color gradient");
    QRadioButton* single_color_btn = new QRadioButton("Choose single color");
    QRadioButton* gradient_btn = new QRadioButton("Load gradient from first row of image file");
    QPushButton *cancel_btn = new QPushButton("Cancel");
    QPushButton *ok_btn = new QPushButton("OK");
    connect(cancel_btn, SIGNAL(pressed()), dlg, SLOT(reject()));
    connect(ok_btn, SIGNAL(pressed()), dlg, SLOT(accept()));
    single_color_btn->setChecked(true);
    ok_btn->setDefault(true);

    QGridLayout *layout0 = new QGridLayout();
    layout0->addWidget(single_color_btn, 0, 0);
    layout0->addWidget(gradient_btn, 1, 0);
    QGridLayout *layout1 = new QGridLayout();
    layout1->addWidget(cancel_btn, 0, 0);
    layout1->addWidget(ok_btn, 0, 1);
    QGridLayout *layout = new QGridLayout();
    layout->addLayout(layout0, 0, 0);
    layout->addLayout(layout1, 1, 0);
    dlg->setLayout(layout);

    dlg->exec();
    if (dlg->result() == QDialog::Accepted) {
        if (single_color_btn->isChecked()) {
            QColor color = QColorDialog::getColor(
                    QColor(gradient[0], gradient[1], gradient[2]), this);
            gradient_length = 1;
            gradient[0] = color.red();
            gradient[1] = color.green();
            gradient[2] = color.blue();
        } else {
            QFileDialog *file_dlg = new QFileDialog(this);
            file_dlg->setDirectory(_settings->value("Session/last-gradient-open-dir", QDir::homePath()).toString());
            file_dlg->setWindowTitle("Open image file");
            file_dlg->setAcceptMode(QFileDialog::AcceptOpen);
            file_dlg->setFileMode(QFileDialog::ExistingFile);
            if (file_dlg->exec()) {
                _settings->setValue("Session/last-gradient-open-dir", file_dlg->directory().path());
                QString file_name = file_dlg->selectedFiles().first();
                QImage img(file_name);
                if (img.isNull()) {
                    QMessageBox::critical(this, "Error", "Cannot open image.");
                } else {
                    QImage line = img.copy(0, 0, img.width(), 1);
                    if (line.width() > processing_parameters::max_gradient_length) {
                        line = line.scaled(processing_parameters::max_gradient_length, 1);
                    }
                    gradient_length = line.width();
                    for (int i = 0; i < gradient_length; i++) {
                        QRgb px = line.pixel(i, 0);
                        gradient[3 * i + 0] = qRed(px);
                        gradient[3 * i + 1] = qGreen(px);
                        gradient[3 * i + 2] = qBlue(px);
                    }
                }
            }
            delete file_dlg;
        }
    }
}

void DBProcessingParameters::update()
{
    const database_description* dd = _master_state->get_database_description(_db_uuid);
    const processing_parameters& pp = dd->processing_parameters[_p_index];

    _active_checkbox->setChecked(dd->active[_p_index]);
    _priority_spinbox->setValue(dd->priority[_p_index]);
    _weight_spinbox->setValue(dd->weight[_p_index]);
    if (pp.category_e2c) {
        _e2c_gradient_label->setPixmap(pixmap_from_gradient(pp.e2c.gradient_length, pp.e2c.gradient));
        _e2c_adapt_brightness_checkbox->setChecked(pp.e2c.adapt_brightness);
        _e2c_isolines_distance_spinbox->setValue(pp.e2c.isolines_distance);
        _e2c_isolines_thickness_spinbox->setValue(pp.e2c.isolines_thickness);
        _e2c_isolines_color_label->setPixmap(pixmap_from_gradient(1, pp.e2c.isolines_color));
    } else if (dd->db.category() == ecmdb::category_elevation) {
        _elevation_scale_factor_spinbox->setValue(pp.elevation.scale_factor);
        _elevation_scale_center_spinbox->setValue(pp.elevation.scale_center);
    } else if (dd->db.category() == ecmdb::category_texture) {
        _texture_contrast_spinbox->setValue(pp.texture.contrast);
        _texture_brightness_spinbox->setValue(pp.texture.brightness);
        _texture_saturation_spinbox->setValue(pp.texture.saturation);
        _texture_hue_spinbox->setValue(pp.texture.hue);
    } else if (dd->db.category() == ecmdb::category_sar_amplitude) {
        _sar_amplitude_despeckling_mean_masksize_spinbox->setValue(pp.sar_amplitude.despeckling.mean.kh * 2 + 1);
        _sar_amplitude_despeckling_median_masksize_spinbox->setValue(pp.sar_amplitude.despeckling.median.kh * 2 + 1);
        _sar_amplitude_despeckling_gauss_sigma_spinbox->setValue(pp.sar_amplitude.despeckling.gauss.sh);
        _sar_amplitude_despeckling_lee_masksize_spinbox->setValue(pp.sar_amplitude.despeckling.lee.kh * 2 + 1);
        _sar_amplitude_despeckling_lee_sigma_spinbox->setValue(pp.sar_amplitude.despeckling.lee.sigma_n);
        _sar_amplitude_despeckling_kuan_masksize_spinbox->setValue(pp.sar_amplitude.despeckling.kuan.kh * 2 + 1);
        _sar_amplitude_despeckling_kuan_l_spinbox->setValue(pp.sar_amplitude.despeckling.kuan.L);
        _sar_amplitude_despeckling_xiao_masksize_spinbox->setValue(pp.sar_amplitude.despeckling.xiao.kh * 2 + 1);
        _sar_amplitude_despeckling_xiao_tmin_spinbox->setValue(pp.sar_amplitude.despeckling.xiao.Tmin);
        _sar_amplitude_despeckling_xiao_tmax_spinbox->setValue(pp.sar_amplitude.despeckling.xiao.Tmax);
        _sar_amplitude_despeckling_xiao_a_spinbox->setValue(pp.sar_amplitude.despeckling.xiao.a);
        _sar_amplitude_despeckling_xiao_b_spinbox->setValue(pp.sar_amplitude.despeckling.xiao.b);
        _sar_amplitude_despeckling_frost_masksize_spinbox->setValue(pp.sar_amplitude.despeckling.frost.kh * 2 + 1);
        _sar_amplitude_despeckling_frost_a_spinbox->setValue(pp.sar_amplitude.despeckling.frost.a);
        _sar_amplitude_despeckling_gammamap_masksize_spinbox->setValue(pp.sar_amplitude.despeckling.gammamap.kh * 2 + 1);
        _sar_amplitude_despeckling_gammamap_l_spinbox->setValue(pp.sar_amplitude.despeckling.gammamap.L);
        _sar_amplitude_despeckling_oddy_masksize_spinbox->setValue(pp.sar_amplitude.despeckling.oddy.kh * 2 + 1);
        _sar_amplitude_despeckling_oddy_alpha_spinbox->setValue(pp.sar_amplitude.despeckling.oddy.alpha);
        _sar_amplitude_despeckling_waveletst_threshold_spinbox->setValue(pp.sar_amplitude.despeckling.waveletst.threshold);
        _sar_amplitude_drr_linear_minamp_spinbox->setValue(pp.sar_amplitude.drr.linear.min_amp);
        _sar_amplitude_drr_linear_minamp_slider->setValue(round(pp.sar_amplitude.drr.linear.min_amp * 1000.0));
        _sar_amplitude_drr_linear_maxamp_spinbox->setValue(pp.sar_amplitude.drr.linear.max_amp);
        _sar_amplitude_drr_linear_maxamp_slider->setValue(round(pp.sar_amplitude.drr.linear.max_amp * 1000.0));
        _sar_amplitude_drr_log_minamp_spinbox->setValue(pp.sar_amplitude.drr.log.min_amp);
        _sar_amplitude_drr_log_minamp_slider->setValue(round(pp.sar_amplitude.drr.log.min_amp * 1000.0));
        _sar_amplitude_drr_log_maxamp_spinbox->setValue(pp.sar_amplitude.drr.log.max_amp);
        _sar_amplitude_drr_log_maxamp_slider->setValue(round(pp.sar_amplitude.drr.log.max_amp * 1000.0));
        _sar_amplitude_drr_log_prescale_spinbox->setValue(pp.sar_amplitude.drr.log.prescale);
        _sar_amplitude_drr_log_prescale_slider->setValue(round(pp.sar_amplitude.drr.log.prescale));
        _sar_amplitude_drr_gamma_minamp_spinbox->setValue(pp.sar_amplitude.drr.gamma.min_amp);
        _sar_amplitude_drr_gamma_minamp_slider->setValue(round(pp.sar_amplitude.drr.gamma.min_amp * 1000.0));
        _sar_amplitude_drr_gamma_maxamp_spinbox->setValue(pp.sar_amplitude.drr.gamma.max_amp);
        _sar_amplitude_drr_gamma_maxamp_slider->setValue(round(pp.sar_amplitude.drr.gamma.max_amp * 1000.0));
        _sar_amplitude_drr_gamma_gamma_spinbox->setValue(pp.sar_amplitude.drr.gamma.gamma);
        _sar_amplitude_drr_gamma_gamma_slider->setValue(pp.sar_amplitude.drr.gamma.gamma < 1.0
                ? round(-300.0 * (1.0 - (pp.sar_amplitude.drr.gamma.gamma - 0.25) / 0.75))
                : round(300.0 * ((pp.sar_amplitude.drr.gamma.gamma - 1.0) / 3.0)));
        _sar_amplitude_drr_schlick_brightness_spinbox->setValue(pp.sar_amplitude.drr.schlick.brightness);
        _sar_amplitude_drr_schlick_brightness_slider->setValue(round(pp.sar_amplitude.drr.schlick.brightness));
        _sar_amplitude_drr_reinhard_brightness_spinbox->setValue(pp.sar_amplitude.drr.reinhard.brightness);
        _sar_amplitude_drr_reinhard_brightness_slider->setValue(round(pp.sar_amplitude.drr.reinhard.brightness * 100.0));
        _sar_amplitude_drr_reinhard_contrast_spinbox->setValue(pp.sar_amplitude.drr.reinhard.contrast);
        _sar_amplitude_drr_reinhard_contrast_slider->setValue(round(pp.sar_amplitude.drr.reinhard.contrast * 1000.0));
        _sar_amplitude_drr_schlicklocal_brightness_spinbox->setValue(pp.sar_amplitude.drr.schlicklocal.brightness);
        _sar_amplitude_drr_schlicklocal_brightness_slider->setValue(round(pp.sar_amplitude.drr.schlicklocal.brightness * 100.0));
        _sar_amplitude_drr_schlicklocal_details_spinbox->setValue(pp.sar_amplitude.drr.schlicklocal.details);
        _sar_amplitude_drr_schlicklocal_details_slider->setValue(round(pp.sar_amplitude.drr.schlicklocal.details * 1000.0));
        _sar_amplitude_drr_schlicklocal_threshold_spinbox->setValue(pp.sar_amplitude.drr.schlicklocal.threshold);
        _sar_amplitude_drr_schlicklocal_threshold_slider->setValue(round(pp.sar_amplitude.drr.schlicklocal.threshold * 1000.0));
        _sar_amplitude_drr_reinhardlocal_brightness_spinbox->setValue(pp.sar_amplitude.drr.reinhardlocal.brightness);
        _sar_amplitude_drr_reinhardlocal_brightness_slider->setValue(round(pp.sar_amplitude.drr.reinhardlocal.brightness * 100.0));
        _sar_amplitude_drr_reinhardlocal_contrast_spinbox->setValue(pp.sar_amplitude.drr.reinhardlocal.contrast);
        _sar_amplitude_drr_reinhardlocal_contrast_slider->setValue(round(pp.sar_amplitude.drr.reinhardlocal.contrast * 1000.0));
        _sar_amplitude_drr_reinhardlocal_details_spinbox->setValue(pp.sar_amplitude.drr.reinhardlocal.details);
        _sar_amplitude_drr_reinhardlocal_details_slider->setValue(round(pp.sar_amplitude.drr.reinhardlocal.details * 1000.0));
        _sar_amplitude_drr_reinhardlocal_threshold_spinbox->setValue(pp.sar_amplitude.drr.reinhardlocal.threshold);
        _sar_amplitude_drr_reinhardlocal_threshold_slider->setValue(round(pp.sar_amplitude.drr.reinhardlocal.threshold * 1000.0));
        _sar_amplitude_despeckling_method_box->setCurrentIndex(pp.sar_amplitude.despeckling_method);
        _sar_amplitude_despeckling_params_stack->setCurrentIndex(pp.sar_amplitude.despeckling_method);
        _sar_amplitude_drr_method_box->setCurrentIndex(pp.sar_amplitude.drr_method);
        _sar_amplitude_drr_params_stack->setCurrentIndex(pp.sar_amplitude.drr_method);
        _sar_amplitude_gradient_label->setPixmap(pixmap_from_gradient(pp.sar_amplitude.gradient_length, pp.sar_amplitude.gradient));
        _sar_amplitude_adapt_brightness_checkbox->setChecked(pp.sar_amplitude.adapt_brightness);
    } else if (dd->db.category() == ecmdb::category_data) {
        _data_offset_spinbox->setValue(pp.data.offset);
        _data_factor_spinbox->setValue(pp.data.factor);
        _data_gradient_label->setPixmap(pixmap_from_gradient(pp.data.gradient_length, pp.data.gradient));
    }
}

void DBProcessingParameters::set_active(bool x)
{
    database_description* dd = _master_state->get_database_description(_db_uuid);
    dd->active[_p_index] = x;
}

void DBProcessingParameters::set_priority(int x)
{
    database_description* dd = _master_state->get_database_description(_db_uuid);
    dd->priority[_p_index] = x;
}

void DBProcessingParameters::set_weight(double x)
{
    database_description* dd = _master_state->get_database_description(_db_uuid);
    dd->weight[_p_index] = x;
}

void DBProcessingParameters::set_elevation_scale_center(double x)
{
    database_description* dd = _master_state->get_database_description(_db_uuid);
    dd->processing_parameters[_p_index].elevation.scale_center = x;
    _lock = true;
    _elevation_scale_center_slider->setValue(round(x / 10.0));
    _lock = false;
}

void DBProcessingParameters::slide_elevation_scale_center(int x)
{
    if (!_lock) _elevation_scale_center_spinbox->setValue(static_cast<double>(x) * 10.0);
}

void DBProcessingParameters::set_elevation_scale_factor(double x)
{
    database_description* dd = _master_state->get_database_description(_db_uuid);
    dd->processing_parameters[_p_index].elevation.scale_factor = x;
    _lock = true;
    _elevation_scale_factor_slider->setValue(round(x * 10.0));
    _lock = false;
}

void DBProcessingParameters::slide_elevation_scale_factor(int x)
{
    if (!_lock) _elevation_scale_factor_spinbox->setValue(static_cast<double>(x) / 10.0);
}

void DBProcessingParameters::set_texture_contrast(double x)
{
    database_description* dd = _master_state->get_database_description(_db_uuid);
    dd->processing_parameters[_p_index].texture.contrast = x;
    _lock = true;
    _texture_contrast_slider->setValue(round(x * 100.0));
    _lock = false;
}

void DBProcessingParameters::slide_texture_contrast(int x)
{
    if (!_lock) _texture_contrast_spinbox->setValue(static_cast<double>(x) / 100.0);
}

void DBProcessingParameters::set_texture_brightness(double x)
{
    database_description* dd = _master_state->get_database_description(_db_uuid);
    dd->processing_parameters[_p_index].texture.brightness = x;
    _lock = true;
    _texture_brightness_slider->setValue(round(x * 100.0));
    _lock = false;
}

void DBProcessingParameters::slide_texture_brightness(int x)
{
    if (!_lock) _texture_brightness_spinbox->setValue(static_cast<double>(x) / 100.0);
}

void DBProcessingParameters::set_texture_saturation(double x)
{
    database_description* dd = _master_state->get_database_description(_db_uuid);
    dd->processing_parameters[_p_index].texture.saturation = x;
    _lock = true;
    _texture_saturation_slider->setValue(round(x * 100.0));
    _lock = false;
}

void DBProcessingParameters::slide_texture_saturation(int x)
{
    if (!_lock) _texture_saturation_spinbox->setValue(static_cast<double>(x) / 100.0);
}

void DBProcessingParameters::set_texture_hue(double x)
{
    database_description* dd = _master_state->get_database_description(_db_uuid);
    dd->processing_parameters[_p_index].texture.hue = x;
    _lock = true;
    _texture_hue_slider->setValue(round(x * 100.0));
    _lock = false;
}

void DBProcessingParameters::slide_texture_hue(int x)
{
    if (!_lock) _texture_hue_spinbox->setValue(static_cast<double>(x) / 100.0);
}

void DBProcessingParameters::set_sar_amplitude_despeckling_method(int x)
{
    database_description* dd = _master_state->get_database_description(_db_uuid);
    _sar_amplitude_despeckling_params_stack->setCurrentIndex(x);
    dd->processing_parameters[_p_index].sar_amplitude.despeckling_method = x;
}

void DBProcessingParameters::set_sar_amplitude_despeckling_mean_masksize(int x)
{
    database_description* dd = _master_state->get_database_description(_db_uuid);
    dd->processing_parameters[_p_index].sar_amplitude.despeckling.mean.kh = (x - 1) / 2;
    dd->processing_parameters[_p_index].sar_amplitude.despeckling.mean.kv = (x - 1) / 2;
}

void DBProcessingParameters::set_sar_amplitude_despeckling_median_masksize(int x)
{
    database_description* dd = _master_state->get_database_description(_db_uuid);
    dd->processing_parameters[_p_index].sar_amplitude.despeckling.median.kh = (x - 1) / 2;
    dd->processing_parameters[_p_index].sar_amplitude.despeckling.median.kv = (x - 1) / 2;
}

void DBProcessingParameters::set_sar_amplitude_despeckling_gauss_sigma(double x)
{
    database_description* dd = _master_state->get_database_description(_db_uuid);
    dd->processing_parameters[_p_index].sar_amplitude.despeckling.gauss.sh = x;
    dd->processing_parameters[_p_index].sar_amplitude.despeckling.gauss.sv = x;
    int k = clamp(static_cast<int>(round(x * 2.5)), 0, 9);
    dd->processing_parameters[_p_index].sar_amplitude.despeckling.gauss.kh = k;
    dd->processing_parameters[_p_index].sar_amplitude.despeckling.gauss.kv = k;
}

void DBProcessingParameters::set_sar_amplitude_despeckling_lee_masksize(int x)
{
    database_description* dd = _master_state->get_database_description(_db_uuid);
    dd->processing_parameters[_p_index].sar_amplitude.despeckling.lee.kh = (x - 1) / 2;
    dd->processing_parameters[_p_index].sar_amplitude.despeckling.lee.kv = (x - 1) / 2;
}

void DBProcessingParameters::set_sar_amplitude_despeckling_lee_sigma(double x)
{
    database_description* dd = _master_state->get_database_description(_db_uuid);
    dd->processing_parameters[_p_index].sar_amplitude.despeckling.lee.sigma_n = x;
}

void DBProcessingParameters::set_sar_amplitude_despeckling_kuan_masksize(int x)
{
    database_description* dd = _master_state->get_database_description(_db_uuid);
    dd->processing_parameters[_p_index].sar_amplitude.despeckling.kuan.kh = (x - 1) / 2;
    dd->processing_parameters[_p_index].sar_amplitude.despeckling.kuan.kv = (x - 1) / 2;
}

void DBProcessingParameters::set_sar_amplitude_despeckling_kuan_l(double x)
{
    database_description* dd = _master_state->get_database_description(_db_uuid);
    dd->processing_parameters[_p_index].sar_amplitude.despeckling.kuan.L = x;
}

void DBProcessingParameters::set_sar_amplitude_despeckling_xiao_masksize(int x)
{
    database_description* dd = _master_state->get_database_description(_db_uuid);
    dd->processing_parameters[_p_index].sar_amplitude.despeckling.xiao.kh = (x - 1) / 2;
    dd->processing_parameters[_p_index].sar_amplitude.despeckling.xiao.kv = (x - 1) / 2;
}

void DBProcessingParameters::set_sar_amplitude_despeckling_xiao_tmin(double x)
{
    database_description* dd = _master_state->get_database_description(_db_uuid);
    dd->processing_parameters[_p_index].sar_amplitude.despeckling.xiao.Tmin = x;
}

void DBProcessingParameters::set_sar_amplitude_despeckling_xiao_tmax(double x)
{
    database_description* dd = _master_state->get_database_description(_db_uuid);
    dd->processing_parameters[_p_index].sar_amplitude.despeckling.xiao.Tmax = x;
}

void DBProcessingParameters::set_sar_amplitude_despeckling_xiao_a(double x)
{
    database_description* dd = _master_state->get_database_description(_db_uuid);
    dd->processing_parameters[_p_index].sar_amplitude.despeckling.xiao.a = x;
}

void DBProcessingParameters::set_sar_amplitude_despeckling_xiao_b(double x)
{
    database_description* dd = _master_state->get_database_description(_db_uuid);
    dd->processing_parameters[_p_index].sar_amplitude.despeckling.xiao.b = x;
}

void DBProcessingParameters::set_sar_amplitude_despeckling_frost_masksize(int x)
{
    database_description* dd = _master_state->get_database_description(_db_uuid);
    dd->processing_parameters[_p_index].sar_amplitude.despeckling.frost.kh = (x - 1) / 2;
    dd->processing_parameters[_p_index].sar_amplitude.despeckling.frost.kv = (x - 1) / 2;
}

void DBProcessingParameters::set_sar_amplitude_despeckling_frost_a(double x)
{
    database_description* dd = _master_state->get_database_description(_db_uuid);
    dd->processing_parameters[_p_index].sar_amplitude.despeckling.frost.a = x;
}

void DBProcessingParameters::set_sar_amplitude_despeckling_gammamap_masksize(int x)
{
    database_description* dd = _master_state->get_database_description(_db_uuid);
    dd->processing_parameters[_p_index].sar_amplitude.despeckling.gammamap.kh = (x - 1) / 2;
    dd->processing_parameters[_p_index].sar_amplitude.despeckling.gammamap.kv = (x - 1) / 2;
}

void DBProcessingParameters::set_sar_amplitude_despeckling_gammamap_l(double x)
{
    database_description* dd = _master_state->get_database_description(_db_uuid);
    dd->processing_parameters[_p_index].sar_amplitude.despeckling.gammamap.L = x;
}

void DBProcessingParameters::set_sar_amplitude_despeckling_oddy_masksize(int x)
{
    database_description* dd = _master_state->get_database_description(_db_uuid);
    dd->processing_parameters[_p_index].sar_amplitude.despeckling.oddy.kh = (x - 1) / 2;
    dd->processing_parameters[_p_index].sar_amplitude.despeckling.oddy.kv = (x - 1) / 2;
}

void DBProcessingParameters::set_sar_amplitude_despeckling_oddy_alpha(double x)
{
    database_description* dd = _master_state->get_database_description(_db_uuid);
    dd->processing_parameters[_p_index].sar_amplitude.despeckling.oddy.alpha = x;
}

void DBProcessingParameters::set_sar_amplitude_despeckling_waveletst_threshold(double x)
{
    database_description* dd = _master_state->get_database_description(_db_uuid);
    dd->processing_parameters[_p_index].sar_amplitude.despeckling.waveletst.threshold = x;
}

void DBProcessingParameters::set_sar_amplitude_drr_method(int x)
{
    database_description* dd = _master_state->get_database_description(_db_uuid);
    _sar_amplitude_drr_params_stack->setCurrentIndex(x);
    dd->processing_parameters[_p_index].sar_amplitude.drr_method = x;
}

void DBProcessingParameters::set_sar_amplitude_drr_linear_minamp(double x)
{
    database_description* dd = _master_state->get_database_description(_db_uuid);
    dd->processing_parameters[_p_index].sar_amplitude.drr.linear.min_amp = x;
    _lock = true;
    _sar_amplitude_drr_linear_minamp_slider->setValue(round(x * 1000.0));
    _lock = false;
}

void DBProcessingParameters::slide_sar_amplitude_drr_linear_minamp(int x)
{
    if (!_lock) _sar_amplitude_drr_linear_minamp_spinbox->setValue(static_cast<double>(x) / 1000.0);
}

void DBProcessingParameters::set_sar_amplitude_drr_linear_maxamp(double x)
{
    database_description* dd = _master_state->get_database_description(_db_uuid);
    dd->processing_parameters[_p_index].sar_amplitude.drr.linear.max_amp = x;
    _lock = true;
    _sar_amplitude_drr_linear_maxamp_slider->setValue(round(x * 1000.0));
    _lock = false;
}

void DBProcessingParameters::slide_sar_amplitude_drr_linear_maxamp(int x)
{
    if (!_lock) _sar_amplitude_drr_linear_maxamp_spinbox->setValue(static_cast<double>(x) / 1000.0);
}

void DBProcessingParameters::set_sar_amplitude_drr_log_minamp(double x)
{
    database_description* dd = _master_state->get_database_description(_db_uuid);
    dd->processing_parameters[_p_index].sar_amplitude.drr.log.min_amp = x;
    _lock = true;
    _sar_amplitude_drr_log_minamp_slider->setValue(round(x * 1000.0));
    _lock = false;
}

void DBProcessingParameters::slide_sar_amplitude_drr_log_minamp(int x)
{
    if (!_lock) _sar_amplitude_drr_log_minamp_spinbox->setValue(static_cast<double>(x) / 1000.0);
}

void DBProcessingParameters::set_sar_amplitude_drr_log_maxamp(double x)
{
    database_description* dd = _master_state->get_database_description(_db_uuid);
    dd->processing_parameters[_p_index].sar_amplitude.drr.log.max_amp = x;
    _lock = true;
    _sar_amplitude_drr_log_maxamp_slider->setValue(round(x * 1000.0));
    _lock = false;
}

void DBProcessingParameters::slide_sar_amplitude_drr_log_maxamp(int x)
{
    if (!_lock) _sar_amplitude_drr_log_maxamp_spinbox->setValue(static_cast<double>(x) / 1000.0);
}

void DBProcessingParameters::set_sar_amplitude_drr_log_prescale(double x)
{
    database_description* dd = _master_state->get_database_description(_db_uuid);
    dd->processing_parameters[_p_index].sar_amplitude.drr.log.prescale = x;
    _lock = true;
    _sar_amplitude_drr_log_prescale_slider->setValue(round(x));
    _lock = false;
}

void DBProcessingParameters::slide_sar_amplitude_drr_log_prescale(int x)
{
    if (!_lock) _sar_amplitude_drr_log_prescale_spinbox->setValue(static_cast<double>(x));
}

void DBProcessingParameters::set_sar_amplitude_drr_gamma_minamp(double x)
{
    database_description* dd = _master_state->get_database_description(_db_uuid);
    dd->processing_parameters[_p_index].sar_amplitude.drr.gamma.min_amp = x;
    _lock = true;
    _sar_amplitude_drr_gamma_minamp_slider->setValue(round(x * 1000.0));
    _lock = false;
}

void DBProcessingParameters::slide_sar_amplitude_drr_gamma_minamp(int x)
{
    if (!_lock) _sar_amplitude_drr_gamma_minamp_spinbox->setValue(static_cast<double>(x) / 1000.0);
}

void DBProcessingParameters::set_sar_amplitude_drr_gamma_maxamp(double x)
{
    database_description* dd = _master_state->get_database_description(_db_uuid);
    dd->processing_parameters[_p_index].sar_amplitude.drr.gamma.max_amp = x;
    _lock = true;
    _sar_amplitude_drr_gamma_maxamp_slider->setValue(round(x * 1000.0));
    _lock = false;
}

void DBProcessingParameters::slide_sar_amplitude_drr_gamma_maxamp(int x)
{
    if (!_lock) _sar_amplitude_drr_gamma_maxamp_spinbox->setValue(static_cast<double>(x) / 1000.0);
}

void DBProcessingParameters::set_sar_amplitude_drr_gamma_gamma(double x)
{
    database_description* dd = _master_state->get_database_description(_db_uuid);
    dd->processing_parameters[_p_index].sar_amplitude.drr.gamma.gamma = x;
    _lock = true;
    if (x < 1.0)
        _sar_amplitude_drr_gamma_gamma_slider->setValue(round(-300.0 * (1.0 - (x - 0.25) / 0.75)));
    else
        _sar_amplitude_drr_gamma_gamma_slider->setValue(round(300.0 * ((x - 1.00) / 3.00)));
    _lock = false;
}

void DBProcessingParameters::slide_sar_amplitude_drr_gamma_gamma(int x)
{
    if (!_lock)
    {
        if (x < 0)
            _sar_amplitude_drr_gamma_gamma_spinbox->setValue((static_cast<double>(x) / 300.0) * 0.75 + 1.0);
        else
            _sar_amplitude_drr_gamma_gamma_spinbox->setValue((static_cast<double>(x) / 300.0) * 3.0 + 1.0);
    }
}

void DBProcessingParameters::set_sar_amplitude_drr_schlick_brightness(double x)
{
    database_description* dd = _master_state->get_database_description(_db_uuid);
    dd->processing_parameters[_p_index].sar_amplitude.drr.schlick.brightness = x;
    _lock = true;
    _sar_amplitude_drr_schlick_brightness_slider->setValue(round(x));
    _lock = false;
}

void DBProcessingParameters::slide_sar_amplitude_drr_schlick_brightness(int x)
{
    if (!_lock) _sar_amplitude_drr_schlick_brightness_spinbox->setValue(static_cast<double>(x));
}

void DBProcessingParameters::set_sar_amplitude_drr_reinhard_brightness(double x)
{
    database_description* dd = _master_state->get_database_description(_db_uuid);
    dd->processing_parameters[_p_index].sar_amplitude.drr.reinhard.brightness = x;
    _lock = true;
    _sar_amplitude_drr_reinhard_brightness_slider->setValue(round(x * 100.0));
    _lock = false;
}

void DBProcessingParameters::slide_sar_amplitude_drr_reinhard_brightness(int x)
{
    if (!_lock) _sar_amplitude_drr_reinhard_brightness_spinbox->setValue(static_cast<double>(x) / 100.0);
}

void DBProcessingParameters::set_sar_amplitude_drr_reinhard_contrast(double x)
{
    database_description* dd = _master_state->get_database_description(_db_uuid);
    dd->processing_parameters[_p_index].sar_amplitude.drr.reinhard.contrast = x;
    _lock = true;
    _sar_amplitude_drr_reinhard_contrast_slider->setValue(round(x * 1000.0));
    _lock = false;
}

void DBProcessingParameters::slide_sar_amplitude_drr_reinhard_contrast(int x)
{
    if (!_lock) _sar_amplitude_drr_reinhard_contrast_spinbox->setValue(static_cast<double>(x) / 1000.0);
}

void DBProcessingParameters::set_sar_amplitude_drr_schlicklocal_brightness(double x)
{
    database_description* dd = _master_state->get_database_description(_db_uuid);
    dd->processing_parameters[_p_index].sar_amplitude.drr.schlicklocal.brightness = x;
    _lock = true;
    _sar_amplitude_drr_schlicklocal_brightness_slider->setValue(round(x * 100.0));
    _lock = false;
}

void DBProcessingParameters::slide_sar_amplitude_drr_schlicklocal_brightness(int x)
{
    if (!_lock) _sar_amplitude_drr_schlicklocal_brightness_spinbox->setValue(static_cast<double>(x) / 100.0);
}

void DBProcessingParameters::set_sar_amplitude_drr_schlicklocal_details(double x)
{
    database_description* dd = _master_state->get_database_description(_db_uuid);
    dd->processing_parameters[_p_index].sar_amplitude.drr.schlicklocal.details = x;
    _lock = true;
    _sar_amplitude_drr_schlicklocal_details_slider->setValue(round(x * 1000.0));
    _lock = false;
}

void DBProcessingParameters::slide_sar_amplitude_drr_schlicklocal_details(int x)
{
    if (!_lock) _sar_amplitude_drr_schlicklocal_details_spinbox->setValue(static_cast<double>(x) / 1000.0);
}

void DBProcessingParameters::set_sar_amplitude_drr_schlicklocal_threshold(double x)
{
    database_description* dd = _master_state->get_database_description(_db_uuid);
    dd->processing_parameters[_p_index].sar_amplitude.drr.schlicklocal.threshold = x;
    _lock = true;
    _sar_amplitude_drr_schlicklocal_threshold_slider->setValue(round(x * 1000.0));
    _lock = false;
}

void DBProcessingParameters::slide_sar_amplitude_drr_schlicklocal_threshold(int x)
{
    if (!_lock) _sar_amplitude_drr_schlicklocal_threshold_spinbox->setValue(static_cast<double>(x) / 1000.0);
}

void DBProcessingParameters::set_sar_amplitude_drr_reinhardlocal_brightness(double x)
{
    database_description* dd = _master_state->get_database_description(_db_uuid);
    dd->processing_parameters[_p_index].sar_amplitude.drr.reinhardlocal.brightness = x;
    _lock = true;
    _sar_amplitude_drr_reinhardlocal_brightness_slider->setValue(round(x * 100.0));
    _lock = false;
}

void DBProcessingParameters::slide_sar_amplitude_drr_reinhardlocal_brightness(int x)
{
    if (!_lock) _sar_amplitude_drr_reinhardlocal_brightness_spinbox->setValue(static_cast<double>(x) / 100.0);
}

void DBProcessingParameters::set_sar_amplitude_drr_reinhardlocal_contrast(double x)
{
    database_description* dd = _master_state->get_database_description(_db_uuid);
    dd->processing_parameters[_p_index].sar_amplitude.drr.reinhardlocal.contrast = x;
    _lock = true;
    _sar_amplitude_drr_reinhardlocal_contrast_slider->setValue(round(x * 1000.0));
    _lock = false;
}

void DBProcessingParameters::slide_sar_amplitude_drr_reinhardlocal_contrast(int x)
{
    if (!_lock) _sar_amplitude_drr_reinhardlocal_contrast_spinbox->setValue(static_cast<double>(x) / 1000.0);
}

void DBProcessingParameters::set_sar_amplitude_drr_reinhardlocal_details(double x)
{
    database_description* dd = _master_state->get_database_description(_db_uuid);
    dd->processing_parameters[_p_index].sar_amplitude.drr.reinhardlocal.details = x;
    _lock = true;
    _sar_amplitude_drr_reinhardlocal_details_slider->setValue(round(x * 1000.0));
    _lock = false;
}

void DBProcessingParameters::slide_sar_amplitude_drr_reinhardlocal_details(int x)
{
    if (!_lock) _sar_amplitude_drr_reinhardlocal_details_spinbox->setValue(static_cast<double>(x) / 1000.0);
}

void DBProcessingParameters::set_sar_amplitude_drr_reinhardlocal_threshold(double x)
{
    database_description* dd = _master_state->get_database_description(_db_uuid);
    dd->processing_parameters[_p_index].sar_amplitude.drr.reinhardlocal.threshold = x;
    _lock = true;
    _sar_amplitude_drr_reinhardlocal_threshold_slider->setValue(round(x * 1000.0));
    _lock = false;
}

void DBProcessingParameters::slide_sar_amplitude_drr_reinhardlocal_threshold(int x)
{
    if (!_lock) _sar_amplitude_drr_reinhardlocal_threshold_spinbox->setValue(static_cast<double>(x) / 1000.0);
}

void DBProcessingParameters::set_sar_amplitude_gradient()
{
    database_description* dd = _master_state->get_database_description(_db_uuid);
    processing_parameters& pp = dd->processing_parameters[_p_index];
    choose_gradient(pp.sar_amplitude.gradient_length, pp.sar_amplitude.gradient);
    _sar_amplitude_gradient_label->setPixmap(pixmap_from_gradient(pp.sar_amplitude.gradient_length, pp.sar_amplitude.gradient));
}

void DBProcessingParameters::set_sar_amplitude_adapt_brightness()
{
    database_description* dd = _master_state->get_database_description(_db_uuid);
    dd->processing_parameters[_p_index].sar_amplitude.adapt_brightness = _sar_amplitude_adapt_brightness_checkbox->isChecked();
}

void DBProcessingParameters::set_data_offset(double x)
{
    database_description* dd = _master_state->get_database_description(_db_uuid);
    dd->processing_parameters[_p_index].data.offset = x;
    _lock = true;
    _data_offset_slider->setValue(round(x * 10.0));
    _lock = false;
}

void DBProcessingParameters::slide_data_offset(int x)
{
    if (!_lock) _data_offset_spinbox->setValue(static_cast<double>(x) / 10.0);
}

void DBProcessingParameters::set_data_factor(double x)
{
    database_description* dd = _master_state->get_database_description(_db_uuid);
    dd->processing_parameters[_p_index].data.factor = x;
    _lock = true;
    _data_factor_slider->setValue(round(x * 10.0));
    _lock = false;
}

void DBProcessingParameters::slide_data_factor(int x)
{
    if (!_lock) _data_factor_spinbox->setValue(static_cast<double>(x) / 10.0);
}

void DBProcessingParameters::set_data_gradient()
{
    database_description* dd = _master_state->get_database_description(_db_uuid);
    processing_parameters& pp = dd->processing_parameters[_p_index];
    choose_gradient(pp.data.gradient_length, pp.data.gradient);
    _data_gradient_label->setPixmap(pixmap_from_gradient(pp.data.gradient_length, pp.data.gradient));
}

void DBProcessingParameters::set_e2c_gradient()
{
    database_description* dd = _master_state->get_database_description(_db_uuid);
    processing_parameters& pp = dd->processing_parameters[_p_index];
    choose_gradient(pp.e2c.gradient_length, pp.e2c.gradient);
    _e2c_gradient_label->setPixmap(pixmap_from_gradient(pp.e2c.gradient_length, pp.e2c.gradient));
}

void DBProcessingParameters::set_e2c_adapt_brightness()
{
    database_description* dd = _master_state->get_database_description(_db_uuid);
    dd->processing_parameters[_p_index].e2c.adapt_brightness = _e2c_adapt_brightness_checkbox->isChecked();
}

void DBProcessingParameters::set_e2c_isolines_distance(double x)
{
    database_description* dd = _master_state->get_database_description(_db_uuid);
    dd->processing_parameters[_p_index].e2c.isolines_distance = x;
}

void DBProcessingParameters::set_e2c_isolines_thickness(double x)
{
    database_description* dd = _master_state->get_database_description(_db_uuid);
    dd->processing_parameters[_p_index].e2c.isolines_thickness = x;
}

void DBProcessingParameters::set_e2c_isolines_color()
{
    database_description* dd = _master_state->get_database_description(_db_uuid);
    processing_parameters& pp = dd->processing_parameters[_p_index];
    QColor color = QColorDialog::getColor(
            QColor(pp.e2c.isolines_color[0], pp.e2c.isolines_color[1], pp.e2c.isolines_color[2]), this);
    pp.e2c.isolines_color[0] = color.red();
    pp.e2c.isolines_color[1] = color.green();
    pp.e2c.isolines_color[2] = color.blue();
    _e2c_isolines_color_label->setPixmap(pixmap_from_gradient(1, pp.e2c.isolines_color));
}


DBDialog::DBDialog(QSettings* settings, state* master_state, const uuid& db_uuid, QWidget* parent) :
    QDialog(parent), _master_state(master_state), _db_uuid(db_uuid)
{
    database_description* dd = master_state->get_database_description(db_uuid);

    _db_info = new DBInfo(*dd);
    _global_processing_parameters = new DBProcessingParameters(settings, false, master_state, db_uuid);
    _lens_processing_parameters = new DBProcessingParameters(settings, true, master_state, db_uuid);

    setWindowTitle(toQString(dd->db.short_description()));
    QGridLayout* layout = new QGridLayout;
    QLabel* label = new QLabel(toQString(dd->db.short_description()));
    layout->addWidget(label, 0, 0);
    _tab = new QTabWidget(this);
    _tab->addTab(_db_info, "Info");
    _tab->addTab(_global_processing_parameters, "Global Parameters");
    _tab->addTab(_lens_processing_parameters, "Lens Parameters");
    layout->addWidget(_tab, 1, 0);
    layout->setRowStretch(1, 1);
    setLayout(layout);
    connect(_lens_processing_parameters->reset_button(), SIGNAL(clicked()), this, SLOT(reset_lens_processing_parameters()));
}

void DBDialog::reset_lens_processing_parameters()
{
    database_description* dd = _master_state->get_database_description(_db_uuid);
    dd->active[1] = dd->active[0];
    dd->priority[1] = dd->priority[0];
    dd->weight[1] = dd->weight[0];
    dd->processing_parameters[1] = dd->processing_parameters[0];
    _lens_processing_parameters->update();
}

/* Items */

ItemDisplayInfo::ItemDisplayInfo()
{
}

ItemDisplayInfo::ItemDisplayInfo(const ItemDisplayInfo& idi) :
    master_state(idi.master_state), db_uuid(idi.db_uuid), display_text(idi.display_text)
{
}

ItemDisplayInfo::ItemDisplayInfo(state* master_state, const uuid& db_uuid, const QString& display_text) :
    master_state(master_state), db_uuid(db_uuid), display_text(display_text)
{
}

QString ItemDisplayInfo::toString() const
{
    std::ostringstream os;
    uintptr_t m = reinterpret_cast<uintptr_t>(master_state);
    s11n::save(os, m);
    s11n::save(os, db_uuid);
    s11n::save(os, display_text.toStdString());
    std::string data_str = os.str();
    QByteArray data(data_str.data(), data_str.length());
    QByteArray b64 = data.toBase64();
    return QString(b64);
}

void ItemDisplayInfo::fromString(const QString& s)
{
    QByteArray b64 = s.toAscii();
    QByteArray data = QByteArray::fromBase64(b64);
    std::string data_str(data.data(), data.length());
    std::istringstream is(data_str);
    uintptr_t m;
    s11n::load(is, m);
    master_state = reinterpret_cast<state*>(m);
    s11n::load(is, db_uuid);
    std::string t;
    s11n::load(is, t);
    display_text = toQString(t);
}

DBItem::DBItem(QSettings* settings, state* master_state, const uuid& db_uuid, const QString& display_text) :
    QStandardItem(display_text),
    master_state(master_state),
    db_uuid(db_uuid),
    db_dialog(master_state ? new DBDialog(settings, master_state, db_uuid) : NULL),
    display_text(display_text)
{
    setEnabled(true);
    setDragEnabled(false);
    setDropEnabled(false);
    setEditable(false);
    setSelectable(true);
    setCheckable(false);
}

DBItem::~DBItem()
{
    delete db_dialog;
}

QVariant DBItem::data(int role) const
{
    if (role == Qt::DisplayRole) {
        return QVariant(ItemDisplayInfo(master_state, db_uuid, display_text).toString());
    } else {
        return QStandardItem::data(role);
    }
}

ItemDelegate::ItemDelegate(QObject* parent) : QItemDelegate(parent)
{
}

void ItemDelegate::drawDisplay(QPainter* painter, const QStyleOptionViewItem& option, const QRect& rect, const QString& text) const
{
    ItemDisplayInfo idi;
    idi.fromString(text);
    QItemDelegate::drawDisplay(painter, option, rect, idi.display_text);
#if 0
    if (idi.weight >= 0.0f)
    {
        painter->save();
        const QRect clearrect(rect.right() - 64, rect.y(), 64, rect.height());
        painter->fillRect(clearrect, (option.state&  QStyle::State_Selected ? option.palette.highlight() : option.palette.base()));
        const QRect barrect(clearrect.x() + 16 + 4, clearrect.y() + 4, clearrect.width() - 16 - 8, clearrect.height() - 8);
        const QRect fillrect(barrect.x(), barrect.y(), barrect.width() - (1.0f - idi.weight) * barrect.width(), barrect.height());
        QLinearGradient gradient(QPointF(barrect.x(), barrect.y()), QPointF(barrect.right(), barrect.y()));
        gradient.setColorAt(0, Qt::red);
        gradient.setColorAt(1, Qt::green);
        painter->fillRect(fillrect, QBrush(gradient));
        painter->drawRect(barrect);
        painter->restore();
    }
#endif
}


/* DBModel */

DBModel::DBModel(QSettings* settings, state* master_state, QObject* parent) :
    QStandardItemModel(parent), _settings(settings), _master_state(master_state)
{
    init();
}

void DBModel::init()
{
    setColumnCount(1);
    setHorizontalHeaderLabels(QStringList(QString("Current data sets")));

    uuid null_uuid;

    _elevation_category_item = new DBItem(_settings, NULL, null_uuid, "Elevation Data");
    _texture_category_item = new DBItem(_settings, NULL, null_uuid, "Surface Textures");
    QStandardItem* parent_item = invisibleRootItem();
    parent_item->appendRow(_elevation_category_item);
    parent_item->appendRow(_texture_category_item);
}

void DBModel::clear()
{
    QStandardItemModel::clear();
    init();
}

void DBModel::add_db(const uuid& db_uuid)
{
    const database_description* dd = _master_state->get_database_description(db_uuid);
    assert(dd);
    DBItem* parent_item =
        (dd->db.category() == ecmdb::category_elevation ? _elevation_category_item : _texture_category_item);
    parent_item->appendRow(new DBItem(_settings, _master_state, db_uuid, toQString(dd->db.short_description())));
}

void DBModel::remove_db(const uuid db_uuid)
{
    for (int i = 0; i < rowCount(); i++) {
        DBItem* dsi0 = reinterpret_cast<DBItem*>(item(i));
        if (dsi0->master_state && dsi0->db_uuid == db_uuid) {
            removeRow(i);
            break;
        }
        for (int j = 0; j < dsi0->rowCount(); j++) {
            DBItem* dsi1 = reinterpret_cast<DBItem*>(dsi0->child(j));
            if (dsi1->master_state && dsi1->db_uuid == db_uuid) {
                dsi0->removeRow(j);
                break;
            }
            for (int k = 0; k < dsi1->rowCount(); k++) {
                DBItem* dsi2 = reinterpret_cast<DBItem*>(dsi1->child(k));
                if (dsi2->master_state && dsi2->db_uuid == db_uuid) {
                    dsi1->removeRow(k);
                    break;
                }
            }
        }
    }
}


/* DBView */

DBView::DBView(QWidget* parent)
    : QTreeView(parent)
{
    //setHeaderHidden(true);
    _item_delegate = new ItemDelegate;
    setItemDelegate(_item_delegate);
}

DBView::~DBView()
{
    delete _item_delegate;
}


/* DBs */

DBs::DBs(QSettings* settings, state* master_state, QWidget* parent) : QWidget(parent), _master_state(master_state)
{
    QGridLayout* layout = new QGridLayout;
    _dbmodel = new DBModel(settings, _master_state, this);
    _dbview = new DBView(this);
    _dbview->setModel(_dbmodel);
    connect(_dbview, SIGNAL(pressed(const QModelIndex&)), this, SLOT(item_rightclicked(const QModelIndex&)));
    connect(_dbview, SIGNAL(doubleClicked(const QModelIndex&)), this, SLOT(item_doubleclicked(const QModelIndex&)));
    layout->addWidget(_dbview, 0, 0);
    layout->setRowStretch(1, 1);
    setLayout(layout);
    update();
}

void DBs::item_doubleclicked(const QModelIndex& index)
{
    DBItem* dbi = reinterpret_cast<DBItem*>(_dbmodel->itemFromIndex(index));
    if (!dbi || !dbi->master_state)
        return;
    dbi->db_dialog->show();
    dbi->db_dialog->raise();
}

void DBs::item_rightclicked(const QModelIndex& index)
{
    if (!(QApplication::mouseButtons() & Qt::RightButton))
        return;
    DBItem* dbi = reinterpret_cast<DBItem*>(_dbmodel->itemFromIndex(index));
    if (!dbi || !dbi->master_state)
        return;
    QMenu* menu = new QMenu(this);
    QAction* dialog_act = menu->addAction("Show dialog...");
    QAction* close_act = NULL;
    if (!(dbi->db_uuid == uuid())) {
        close_act = menu->addAction("Close");
    }
    QAction* chosen_act = menu->exec(QCursor::pos());
    if (chosen_act == dialog_act) {
        dbi->db_dialog->show();
        dbi->db_dialog->raise();
    } else if (chosen_act == close_act && !(dbi->db_uuid == uuid())) {
        close(dbi->db_uuid);
    }
}

void DBs::open(const std::string& url, const std::string& username, const std::string& password)
{
    uuid db_uuid;
    bool reset_viewer = !_master_state->have_databases();
    try {
        db_uuid = _master_state->open_database(url, username, password);
    }
    catch (exc& e) {
        QMessageBox::critical(this, PACKAGE_NAME ": Error", e.what());
        return;
    }
    if (reset_viewer)
        _master_state->reset_viewer();
    _dbmodel->add_db(db_uuid);
}

void DBs::close(const uuid& db_uuid)
{
    _dbmodel->remove_db(db_uuid);
    _master_state->close_database(db_uuid);
}

void DBs::update()
{
    _dbmodel->clear();
    for (size_t i = 0; i < _master_state->database_descriptions.size(); i++) {
        _dbmodel->add_db(_master_state->database_descriptions[i].uuid);
    }
    _dbview->expandAll();
}

void DBs::close_dialogs()
{
    for (int i = 0; i < _dbmodel->rowCount(); i++) {
        DBItem* dbi0 = reinterpret_cast<DBItem*>(_dbmodel->item(i));
        for (int j = 0; j < dbi0->rowCount(); j++) {
            DBItem* dbi1 = reinterpret_cast<DBItem*>(dbi0->child(j));
            for (int k = 0; k < dbi1->rowCount(); k++) {
                DBItem* dbi2 = reinterpret_cast<DBItem*>(dbi1->child(k));
                if (dbi2->db_dialog)
                    dbi2->db_dialog->close();
            }
            if (dbi1->db_dialog)
                dbi1->db_dialog->close();
        }
        if (dbi0->db_dialog)
            dbi0->db_dialog->close();
    }
}
