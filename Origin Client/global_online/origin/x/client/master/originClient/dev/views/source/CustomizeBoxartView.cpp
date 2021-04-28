/////////////////////////////////////////////////////////////////////////////
// CustomizeBoxartView.cpp
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "CustomizeBoxartView.h"
#include "ImageProcessingView.h"

#include "engine/content/ContentController.h"
#include "engine/content/CustomBoxartController.h"
#include "engine/login/LoginController.h"
#include "engine/content/ProductArt.h"

#include "services/debug/DebugService.h"
#include "services/network/GlobalConnectionStateMonitor.h"
#include "services/platform/PlatformService.h"
#include "EbisuSystemTray.h"

#include "TelemetryAPIDLL.h"

#include "originwindow.h"
#include "originwindowmanager.h"
#include "originmsgbox.h"
#include "originscrollablemsgbox.h"
#include "originspinner.h"
#include "originlabel.h"
#include "originpushbutton.h"
#include "originbuttonbox.h"
#include "originwebview.h"
#include "ui_CustomizeBoxart.h"

#include <QDesktopServices>
#include <QDialogButtonBox>
#include <QFileDialog>

#include <QWebPage>
#include <QWebFrame>

#if defined(ORIGIN_PC)
#include <ShlObj.h>
#endif

namespace Origin
{
	namespace Client
	{

       // Design wanted to preview to be 141x200 - which is 61% of 231x326.
       const double CustomizeBoxartView::sPreviewScaling = 0.61;
	   const int ICON_SIZE = 32;
	   const QSize MAX_IMAGE_SIZE(2001, 1001);


        CustomizeBoxartView::CustomizeBoxartView(QWidget *parent) :
            QWidget(parent),
            mMessageBox(NULL),
            mIsNonOriginGame(false),
            mPreviewWebView(NULL)

        {
            // The message box itself is the only transient part
            // of the boxart flow, so it's what needs to
            // be destroyed when the user logs out.
            ORIGIN_VERIFY_CONNECT(Engine::LoginController::instance(), SIGNAL(userLoggedOut(Origin::Engine::UserRef)), this, SLOT(onCanceled()));
        }

        CustomizeBoxartView::~CustomizeBoxartView()
        {

        }

        void CustomizeBoxartView::initialize()
        {

        }

        void CustomizeBoxartView::focus()
        {
            if(mMessageBox)
            {
                mMessageBox->raise();
                mMessageBox->activateWindow();
            }
        }

        void CustomizeBoxartView::showInvalidFileTypeDialog(const QString& fileName)
        {
            UIToolkit::OriginWindow* msgBox = new UIToolkit::OriginWindow(UIToolkit::OriginWindow::Icon | UIToolkit::OriginWindow::Close,
                NULL, UIToolkit::OriginWindow::MsgBox, QDialogButtonBox::Close);
            msgBox->setTitleBarText(tr("application_name"));

            msgBox->msgBox()->setup( UIToolkit::OriginMsgBox::Error, 
                tr("ebisu_client_image_load_failed_title"), tr("ebisu_client_image_load_failed_text").arg(fileName));
            msgBox->button(QDialogButtonBox::Close)->setText(tr("ebisu_client_close"));

            msgBox->manager()->setupButtonFocus();
            msgBox->setAttribute(Qt::WA_DeleteOnClose, false);
            ORIGIN_VERIFY_CONNECT(msgBox, SIGNAL(rejected()), msgBox, SLOT(close()));
            ORIGIN_VERIFY_CONNECT(msgBox->button(QDialogButtonBox::Close), SIGNAL(clicked()), msgBox, SLOT(close()));

            msgBox->exec();
            msgBox->deleteLater();
        }

        void CustomizeBoxartView::onBoxartBrowseSelected()
        {          
            QString initialDirectory;
            #if defined(ORIGIN_PC)
                WCHAR buffer[MAX_PATH];
                SHGetFolderPathW(NULL, CSIDL_MYPICTURES, NULL, SHGFP_TYPE_CURRENT, buffer );
                initialDirectory = QString::fromWCharArray(buffer);
            #elif defined(ORIGIN_MAC)
                initialDirectory = Origin::Services::PlatformService::getStorageLocation(QStandardPaths::PicturesLocation);
            #else
                #error Must specialize for other platform.
            #endif
            if (!QDir(initialDirectory).exists())
            {
                // QFileDialog will default to the working directory
                initialDirectory.clear();
            }

            QString filter = tr("Images (*.png *.jpg *.jpeg *.gif *.bmp)");

            QString filename = QFileDialog::getOpenFileName(mMessageBox, "", initialDirectory, filter);
            if (!filename.isEmpty())
            {
				// Need to make sure we've cleaned up any old temp files
				// The user may have choosen a new file before finishing the first one.
				if(mCurrentBoxartFile.contains(Engine::Content::CustomBoxartController::NEW_IMAGE_TAG))
				{
					// clean up after ourseves and remove the "newly created cropped box art file.
					QFile newBoxartFile(mCurrentBoxartFile);				
					newBoxartFile.remove();
				}

                if (isValidImageFile(filename))
                {
					mCurrentBoxartBaseFile = filename;
					mBoxartData.setUsesDefaultBoxart(false);

					QImage image(filename);
					if (image.width() < ICON_SIZE || image.height() < ICON_SIZE)
					{
						mCurrentBoxartFile = filename;
						mBoxartData.setBoxartDisplayArea(0, 0, image.width(), image.height());
						onUpdateBoxartPreview(filename);
					}
					else
					{
	                    mBoxartData.setBoxartDisplayArea(0, 0, 0, 0);	                    
		                onCropImageSelected();
					}
                }
                else
                {
                    showInvalidFileTypeDialog(QFileInfo(filename).fileName());
                }
            }
        }

        bool CustomizeBoxartView::isValidImageFile(const QString& imageFile)
        {
			QImage* image = new QImage(imageFile);
			if(!image->isNull())
			{
				int width = image->width();
				int height = image->height();
				
				if(width > MAX_IMAGE_SIZE.width() || height > MAX_IMAGE_SIZE.height())
					return false;
			}

            QPixmap pixmap;
            return pixmap.load(imageFile);
        }

        void CustomizeBoxartView::showCustomizeBoxartDialog(Engine::Content::EntitlementRef eRef)
        {
            if (!eRef.isNull())
            {
                mDefaultBoxart = Engine::Content::CustomBoxartController::createDefaultBoxartImage(eRef);
                mIsNonOriginGame = eRef->contentConfiguration()->isNonOriginGame();

                GetTelemetryInterface()->Metric_CUSTOMIZE_BOXART_START(mIsNonOriginGame);

                if (!mIsNonOriginGame)
                {
                    QStringList urls;
                    urls << Engine::Content::ProductArt::urlsForType(eRef->contentConfiguration(), Engine::Content::ProductArt::HighResBoxArt);
                    urls << Engine::Content::ProductArt::urlsForType(eRef->contentConfiguration(), Engine::Content::ProductArt::LowResBoxArt);
                    if (!urls.isEmpty())
                    {
                        mBoxartDefaultUrl = QUrl(urls.front());
                    }
                }

                mBoxartData.setProductId(eRef->contentConfiguration()->productId());
                Origin::Engine::Content::ContentController * c = Engine::Content::ContentController::currentUserContentController();
                if (!c)
                    return;

                c->boxartController()->getBoxartData(eRef->contentConfiguration()->productId(), mBoxartData);

                if (!QFile::exists(mBoxartData.getBoxartSource()))
                {
                    if (mIsNonOriginGame)
                    {
                        mBoxartData.setBoxartSource(Engine::Content::CustomBoxartController::createDefaultBoxart(eRef));
                    }
                    mBoxartData.setUsesDefaultBoxart(true);
                }

                Ui::CustomizeBoxart ui;
                QWidget* content = initializeCustomizeBoxartDialog(ui);

                if (mBoxartData.usesDefaultBoxart())
                {
                    ui.btnCrop->setEnabled(false);
                    ui.btnRestore->setEnabled(false);
                }
             
                mCurrentBoxartFile = mBoxartData.getBoxartSource();
				mCurrentBoxartBaseFile = mBoxartData.getBoxartBase();
                if (!mCurrentBoxartFile.isEmpty() && !mBoxartData.usesDefaultBoxart())
					onUpdateBoxartPreview(mCurrentBoxartFile);
				else if(!mCurrentBoxartBaseFile.isEmpty() && !mBoxartData.usesDefaultBoxart())
					onUpdateBoxartPreview(mCurrentBoxartBaseFile);
                else if (mBoxartDefaultUrl.isValid())
                    onUpdateBoxartPreview(mBoxartDefaultUrl);
                else
                    onUpdateBoxartPreview(mDefaultBoxart);

                mMessageBox->setContent(content);

                mMessageBox->showWindow();
            }
        }

        QWidget* CustomizeBoxartView::initializeCustomizeBoxartDialog(Ui::CustomizeBoxart& ui)
        {
            mMessageBox = new UIToolkit::OriginWindow(UIToolkit::OriginWindow::Icon | UIToolkit::OriginWindow::Minimize | UIToolkit::OriginWindow::Close,
                    NULL, UIToolkit::OriginWindow::ScrollableMsgBox, QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
            mMessageBox->scrollableMsgBox()->setup(UIToolkit::OriginScrollableMsgBox::NoIcon, tr("ebisu_client_customize_box_art_uppercase"), tr("ebisu_client_customize_boxart_description"));

            QWidget* commandlinkContent = new QWidget();
            ui.setupUi(commandlinkContent);

            mPreviewWebView = ui.previewWebView;
            mPreviewWebView->page()->mainFrame()->setScrollBarPolicy(Qt::Horizontal, Qt::ScrollBarAlwaysOff);

            #if defined(ORIGIN_MAC)
                ui.verticalLayout->setSpacing(11);
                ui.gridLayout->setVerticalSpacing(15);
            #endif
         
            ORIGIN_VERIFY_CONNECT(ui.btnRestore, SIGNAL(clicked()), this, SLOT(onRestoreDefaultBoxart()));
            ORIGIN_VERIFY_CONNECT(ui.btnBrowseBoxart, SIGNAL(clicked()), this, SLOT(onBoxartBrowseSelected()));
            ORIGIN_VERIFY_CONNECT(this, SIGNAL(enableRestoreButton(bool)), ui.btnRestore, SLOT(setEnabled(bool)));
            ORIGIN_VERIFY_CONNECT(ui.btnCrop, SIGNAL(clicked()), this, SLOT(onCropImageSelected()));
            ORIGIN_VERIFY_CONNECT(this, SIGNAL(enableCropButton(bool)), ui.btnCrop, SLOT(setEnabled(bool)));


            ORIGIN_VERIFY_CONNECT(mMessageBox->button(QDialogButtonBox::Ok), SIGNAL(clicked()), this, SLOT(onBoxartSet()));
            ORIGIN_VERIFY_CONNECT(mMessageBox->button(QDialogButtonBox::Ok), SIGNAL(clicked()), this, SLOT(sendCustomizeBoxartApplyTelemetry()));
            ORIGIN_VERIFY_CONNECT(mMessageBox->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), this, SLOT(onCanceled()));
            ORIGIN_VERIFY_CONNECT(mMessageBox->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), this, SLOT(sendCustomizeBoxartCancelTelemetry()));
            ORIGIN_VERIFY_CONNECT(mMessageBox, SIGNAL(rejected()), this, SLOT(onCanceled()));
            ORIGIN_VERIFY_CONNECT(mMessageBox, SIGNAL(rejected()), this, SLOT(sendCustomizeBoxartCancelTelemetry()));
            
            mMessageBox->setAttribute(Qt::WA_DeleteOnClose, false);
            if(mMessageBox->manager())
                mMessageBox->manager()->setupButtonFocus();

            return commandlinkContent;
        }

        void CustomizeBoxartView::onCropImageSelected()
        {
            //close customize boxart window
            onAccepted();

            mMessageBox = new UIToolkit::OriginWindow(UIToolkit::OriginWindow::Icon | UIToolkit::OriginWindow::Minimize | UIToolkit::OriginWindow::Close,
                NULL, UIToolkit::OriginWindow::ScrollableMsgBox, QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
            mMessageBox->scrollableMsgBox()->setHasDynamicWidth(true);
            mMessageBox->scrollableMsgBox()->setTitle(tr("ebisu_client_crop_image_uppercase"));
            mMessageBox->setButtonText(QDialogButtonBox::Ok, tr("ebisu_client_crop"));

            UIToolkit::OriginWebView* webview = new UIToolkit::OriginWebView();

            ImageProcessingView* cropView = NULL;
            if (!mCurrentBoxartBaseFile.isEmpty())
            {
                cropView = new ImageProcessingView(mCurrentBoxartBaseFile);
            }
			else if (!mCurrentBoxartFile.isEmpty())
            {
                cropView = new ImageProcessingView(mCurrentBoxartFile);
            }
            else
            {
                cropView = new ImageProcessingView(mDefaultBoxart);
            }
            ORIGIN_VERIFY_CONNECT(cropView, SIGNAL(cropImageLoaded()), this, SLOT(onCropImageLoaded()));

            // TODO: Get the setIsContent size to work.
            // Crop box container + spacing + preview image width
            const int contentWidth = 389 + 20 + Engine::Content::CustomBoxartController::sBoxartWidth;
            webview->setFixedSize(contentWidth, Engine::Content::CustomBoxartController::sBoxartHeight);
            
            webview->setIsContentSize(false);
            QPalette palette = webview->webview()->palette();
            palette.setBrush(QPalette::Base, Qt::transparent);
            cropView->setPalette(palette);
            cropView->setAttribute(Qt::WA_OpaquePaintEvent, false);

            webview->setWebview(cropView);
            webview->webview()->page()->setPalette(palette);
            webview->setVScrollPolicy(Qt::ScrollBarAlwaysOff);
            webview->setHScrollPolicy(Qt::ScrollBarAlwaysOff);
            mMessageBox->setContent(webview);
            if(mMessageBox->manager())
                mMessageBox->manager()->setupButtonFocus();
            ORIGIN_VERIFY_CONNECT(mMessageBox->button(QDialogButtonBox::Ok), SIGNAL(clicked()), this, SLOT(onCropBoxart()));
            ORIGIN_VERIFY_CONNECT(mMessageBox->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), this, SLOT(onCanceled()));
            ORIGIN_VERIFY_CONNECT(mMessageBox, SIGNAL(rejected()), this, SLOT(onCanceled()));
        }

        void CustomizeBoxartView::onCropImageLoaded()
        {
            if (mMessageBox)
            {
                UIToolkit::OriginWebView* webview = dynamic_cast<UIToolkit::OriginWebView*>(mMessageBox->content());
                if (webview)
                {
                    ImageProcessingView* cropView = dynamic_cast<ImageProcessingView*>(webview->webview());
                    if (cropView)
                    {
                        ORIGIN_VERIFY_DISCONNECT(cropView, SIGNAL(cropImageLoaded()), this, SLOT(onCropImageLoaded()));
                        mMessageBox->showWindow();
                    }
                }
            }
        }

        void CustomizeBoxartView::onCropBoxart()
        {
            QRect cropArea;
            if(mMessageBox && mMessageBox->content())
            {
                UIToolkit::OriginWebView* webview = dynamic_cast<UIToolkit::OriginWebView*>(mMessageBox->content());
                if(webview && webview->webview())
                {
                    ImageProcessingView* imageProcessingView = dynamic_cast<ImageProcessingView*>(webview->webview());
                    cropArea = imageProcessingView->cropArea();
                }
            }

            if(cropArea.isValid())
            {
				// Only make as accepted once we know we have a valid cropArea
	            //close cropping dialog
	            onAccepted();
	
				// If we do not have a base file user the current file instead.
				// We will never hav e base file to GIF's because we do not create a new file
				if (mCurrentBoxartBaseFile.isEmpty())
					mCurrentBoxartBaseFile = mCurrentBoxartFile;
	            QFileInfo imageFileInfo(mCurrentBoxartBaseFile);
                QString extension = imageFileInfo.suffix();
				// We need to handle GIF filed differently
				// If we create a new file from an animated GIF we lose the animation
				if(extension.compare("gif",Qt::CaseInsensitive) == 0)
				{
					mBoxartData.setBoxartDisplayArea(cropArea);
					mBoxartData.setUsesDefaultBoxart(false);
					mCurrentBoxartFile = mCurrentBoxartBaseFile;
					mCurrentBoxartBaseFile = "";
				}
				// All other files need to be croped and have new file created 
				// This is to help with performace issues with large files
				else
				{
					QImage baseImage;
					baseImage.load(mCurrentBoxartBaseFile);
					QImage newImage = baseImage.copy(cropArea);

					QString name = imageFileInfo.fileName();
					QString path = Engine::Content::CustomBoxartController::getBoxartCachePath();

					mCurrentBoxartFile = path + name;
					mCurrentBoxartFile.insert(mCurrentBoxartFile.lastIndexOf("."), Engine::Content::CustomBoxartController::NEW_IMAGE_TAG);
					newImage.save(mCurrentBoxartFile, extension.toUtf8());
					mBoxartData.setBoxartDisplayArea(QRect());
				}
                Ui::CustomizeBoxart ui;
                QWidget* content = initializeCustomizeBoxartDialog(ui);
                if(!onUpdateBoxartPreview(mCurrentBoxartFile))
				{
					mCurrentBoxartFile = mCurrentBoxartBaseFile;
					mBoxartData.setBoxartDisplayArea(QRect());
					if(!onUpdateBoxartPreview(mCurrentBoxartFile))
					{
						onUpdateBoxartPreview(mDefaultBoxart);
					}
				}
                mMessageBox->setContent(content);
                mMessageBox->showWindow();
            }
			// Should never hit this but just in case
			else
				onCanceled();
        }

        void CustomizeBoxartView::onRestoreDefaultBoxart()
        {
            mBoxartData.setBoxartDisplayArea(0, 0, 0, 0);
            mBoxartData.setUsesDefaultBoxart(true);

            if (mBoxartDefaultUrl.isValid() && Services::Network::GlobalConnectionStateMonitor::instance()->isOnline())
            {
                onUpdateBoxartPreview(mBoxartDefaultUrl);
            }
            else
            {
                onUpdateBoxartPreview(mDefaultBoxart);
            }

            mCurrentBoxartFile.clear();

            mMessageBox->setDefaultButton(QDialogButtonBox::Ok);
        }

        void CustomizeBoxartView::onUpdateBoxartPreview(const QUrl& url)
        {
            if (url.isValid())
            {
                mBoxartData.setBoxartSource("");
                mBoxartData.setBoxartDisplayArea(QRect());
                
                ORIGIN_VERIFY_CONNECT(mPreviewWebView->page()->networkAccessManager(), SIGNAL(finished(QNetworkReply*)), this,
                    SLOT(onLoadDefaultBoxartUrlComplete(QNetworkReply*)));

                mPreviewWebView->setHtml(previewHtml(url.toString(), Engine::Content::CustomBoxartController::sBoxartWidth,
                    Engine::Content::CustomBoxartController::sBoxartHeight));

                emit enableRestoreButton(!mBoxartData.usesDefaultBoxart());
                emit enableCropButton(!mBoxartData.usesDefaultBoxart());
            }
        }

        void CustomizeBoxartView::onLoadDefaultBoxartUrlComplete(QNetworkReply* reply)
        {
            if (reply)
            {
                QString url =  reply->url().toString().toLower();
                const QString lowercaseReplyHost = reply->url().host().toLower();

                if (lowercaseReplyHost.endsWith(".akamaihd.net") && url.endsWith(".jpg"))
                {
                    ORIGIN_VERIFY_DISCONNECT(mPreviewWebView->page()->networkAccessManager(), SIGNAL(finished(QNetworkReply*)), this,
                        SLOT(onLoadDefaultBoxartUrlComplete(QNetworkReply*)));

                    if (reply->error() == QNetworkReply::NoError)
                    {
                        // 200-like response;
                    }
                    else if (reply->error() == QNetworkReply::OperationCanceledError)
                    {
                        // Qt uses this for 304s - ignore
                    }
                    else
                    {
                        onUpdateBoxartPreview(mDefaultBoxart);
                    }
                }
            }
        }

        bool CustomizeBoxartView::onUpdateBoxartPreview(const QString& filename)
        {            
            QFile imageFile(filename);
            if (imageFile.open(QIODevice::ReadOnly))
            {
                mBoxartData.setBoxartSource(filename);
                
                QFileInfo imageFileInfo(filename);
                QString extension = imageFileInfo.suffix();

                QPixmap pixmap;
                pixmap.load(filename);
                int imageWidth = pixmap.width();
                int imageHeight = pixmap.height();

                QByteArray byteArray;
                QBuffer buffer(&byteArray);
                pixmap.save(&buffer, extension.toUtf8());

                QString dataSource = QString("data:image/%1;base64,").arg(extension) + imageFile.readAll().toBase64();
                mPreviewWebView->setHtml(previewHtml(dataSource, imageWidth, imageHeight));

                imageFile.close();

                emit enableRestoreButton(!mBoxartData.usesDefaultBoxart());
				
				// Is the image larger than an icon
				if(imageWidth > ICON_SIZE && imageHeight > ICON_SIZE)
				{
					emit enableCropButton(!mBoxartData.usesDefaultBoxart());
				}
				else if(!mCurrentBoxartBaseFile.isEmpty()) //if not do we have a base image file
				{
					QPixmap basePixmap;
					basePixmap.load(mCurrentBoxartBaseFile);
					// Is our base file larger than an icon
					if(basePixmap.width() > ICON_SIZE && basePixmap.height() > ICON_SIZE)
						emit enableCropButton(!mBoxartData.usesDefaultBoxart());
					else
						emit enableCropButton(false);
				}
				else
					emit enableCropButton(false);

				return true;
            }
			return false;
        }

        void CustomizeBoxartView::onUpdateBoxartPreview(const QPixmap& pixmap)
        {
            if (!pixmap.isNull())
            {
                mBoxartData.setBoxartSource("");
                mBoxartData.setBoxartDisplayArea(QRect());

                int imageWidth = pixmap.width();
                int imageHeight = pixmap.height();

                QByteArray byteArray;
                QBuffer buffer(&byteArray);
                pixmap.save(&buffer, "png");

                QString dataSource = "data:image/png;base64," + byteArray.toBase64();
                mPreviewWebView->setHtml(previewHtml(dataSource, imageWidth, imageHeight));

                emit enableRestoreButton(!mBoxartData.usesDefaultBoxart());
                emit enableCropButton(!mBoxartData.usesDefaultBoxart());

            }
        }

        void CustomizeBoxartView::onBoxartSet()
        {
            Origin::Engine::Content::ContentController * c = Engine::Content::ContentController::currentUserContentController();
            if (c)
            {
                if (!mCurrentBoxartFile.isEmpty())
                {

                    mCurrentBoxartFile = c->boxartController()->saveBoxartSource(mBoxartData.getProductId(), mCurrentBoxartFile);
                    mBoxartData.setBoxartSource(mCurrentBoxartFile);

                    mCurrentBoxartBaseFile = c->boxartController()->saveBoxartBase(mBoxartData.getProductId(), mCurrentBoxartBaseFile);
                    mBoxartData.setBoxartBase(mCurrentBoxartBaseFile);

                    emit setBoxart(mBoxartData);              
                }
                else if (!mDefaultBoxart.isNull())
                {
                    if (mIsNonOriginGame)
                    {
                        mCurrentBoxartFile = c->boxartController()->saveBoxartSource(mBoxartData.getProductId(), mDefaultBoxart);
                        mBoxartData.setBoxartSource(mCurrentBoxartFile);
                        emit setBoxart(mBoxartData);
                    }
                    else
                    {
                        emit removeBoxart();
                    }               
                }
            }
            onAccepted();
        }

        void CustomizeBoxartView::onAccepted()
        {
            if (mMessageBox)
            {
                mMessageBox->accept();
                mMessageBox->deleteLater();
                mMessageBox = NULL;
            }
        }

        void CustomizeBoxartView::onCanceled()
        {
            if (mMessageBox)
            {
                ORIGIN_VERIFY_DISCONNECT(mMessageBox, SIGNAL(rejected()), 0, 0);
                mMessageBox->close();
                mMessageBox->deleteLater();
                mMessageBox = NULL;
            }

			if(mCurrentBoxartFile.contains(Engine::Content::CustomBoxartController::NEW_IMAGE_TAG))
			{
				// clean up after ourseves and remove the "newly created cropped box art file.
				QFile newBoxartFile(mCurrentBoxartFile);				
				newBoxartFile.remove();
			}

            emit cancel();
        }

        QString CustomizeBoxartView::previewHtml(const QString& imageUrl, int imageWidth, int imageHeight)
        {
            int containerHeight = static_cast<int>(sPreviewScaling * Engine::Content::CustomBoxartController::sBoxartHeight + 0.5);
            int containerWidth = static_cast<int>(sPreviewScaling * Engine::Content::CustomBoxartController::sBoxartWidth + 0.5);          

            QString previewContainer = QString("<div style=\"margin: 0px; overflow: hidden; width: %1px; height: %2px;\">").arg(
                containerWidth).arg(
                containerHeight);

            QString html = "<html><body style=\"margin: 0px;\">%1<img style=\"%2\" src=\"%3\"></div></body></html>";

            QString imageStyle;
            if (mBoxartData.getBoxartDisplayArea().isEmpty())
            {
                imageStyle = QString("width: inherit; height: inherit; webkit-user-select: none;");              
            }
            else
            {
                int cropWidth = mBoxartData.getBoxartDisplayArea().width();
                int cropLeft = mBoxartData.getBoxartDisplayArea().left();
                int cropCenterX = cropLeft + static_cast<int>(cropWidth / 2.0 + 0.5);
                int imgCenterX = static_cast<int>(imageWidth / 2.0 + 0.5);
                int centeredLeft = static_cast<int>((containerWidth - imageWidth) / 2.0 + 0.5);
                int dx = centeredLeft - (cropCenterX - imgCenterX);
                
                int cropHeight = mBoxartData.getBoxartDisplayArea().height();
                int cropTop = mBoxartData.getBoxartDisplayArea().top();
                int cropCenterY = cropTop + static_cast<int>(cropHeight / 2.0 + 0.5);
                int imgCenterY = static_cast<int>(imageHeight / 2.0 + 0.5);
                int centeredTop = static_cast<int>((containerHeight - imageHeight) / 2.0 + 0.5);
                int dy = centeredTop - (cropCenterY - imgCenterY);

                double scale = static_cast<double>(containerWidth) / static_cast<double>(mBoxartData.getBoxartDisplayArea().width());

                imageStyle = QString("-webkit-user-select: none; -webkit-transform-origin: %1px %2px; -webkit-transform: translate(%5px, %6px) scale(%3, %4);").arg(
                    cropCenterX).arg(
                    cropCenterY).arg(
                    scale).arg(
                    scale).arg(
                    dx).arg(
                    dy);
            }
            html = html.arg(previewContainer).arg(imageStyle).arg(imageUrl);

            return html;
        }

        void CustomizeBoxartView::sendCustomizeBoxartApplyTelemetry()
        {
            GetTelemetryInterface()->Metric_CUSTOMIZE_BOXART_APPLY(mIsNonOriginGame);
        }

        void CustomizeBoxartView::sendCustomizeBoxartCancelTelemetry()
        {
            GetTelemetryInterface()->Metric_CUSTOMIZE_BOXART_CANCEL(mIsNonOriginGame);
        }
	}
}
