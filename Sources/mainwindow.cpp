#include "mainwindow.h"
#include "./ui_mainwindow.h"

/*
    Constructor of MainWindow.

    Responsibilities here:
    - Initialize base QMainWindow
    - Allocate and setup UI
    - Set window properties (icon, title)
    - Connect background worker signals
    - Apply default theme
*/
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    /*
        Setup UI elements created in Qt Designer (.ui file).

        This initializes:
        - buttons
        - labels
        - input fields
        - menus
    */
    ui->setupUi(this);

    // Set application window icon from Qt resources
    QIcon app_icon(":/resc/icon.ico");
    this->setWindowIcon(app_icon);

    // Set window title
    this->setWindowTitle("File Organizer");

    // Apply default (light) theme at startup
    QString theme_path = ":/styles/light.qss";
    apply_theme(theme_path);

    /*
        Connect the QFutureWatcher signal to our slot.

        Meaning:
        - When the background organizing task finishes
        - Qt automatically calls on_organization_finished()

        This is thread-safe and runs in the GUI thread.
    */
    connect(
        &result_watcher,
        &QFutureWatcher<organize_status>::finished,
        this,
        &MainWindow::on_organization_finished
    );

    ui->progress_bar->setVisible(false);
}

/*
    Destructor of MainWindow.

    Cleans up UI resources to prevent memory leaks.
*/
MainWindow::~MainWindow()
{
    delete ui;
}

/*
    Slot triggered when the "Organize" button is clicked.
*/
void MainWindow::on_organize_button_clicked()
{
    // Default transfer mode is atomic (rename-based move)
    current_mode = transfer_mode::atomic_transfer_mode;

    // Read directory path from input field
    QString q_root_path = ui->path_field->text();

    // Convert QString to std::string for backend compatibility
    std::string root_path = q_root_path.toStdString();

    // If path is empty, do nothing
    if (root_path.empty())
    {
        return;
    }

    /*
        Disable the organize button to:
        - Prevent duplicate clicks
        - Avoid running multiple jobs at once
    */
    ui->organize_button->setEnabled(false);

    ui->progress_bar->setVisible(true); // Show the bar
    ui->progress_bar->setRange(0, 0);

    // Show progress message
    ui->result_field->setText("Organizing...");

    /*
        Run the organize_directory function in a background thread.

        QtConcurrent::run():
        - Executes function asynchronously
        - Returns a QFuture object to track the result
    */
    QFuture<organize_status> future_result =
        QtConcurrent::run(organize_directory, root_path, current_mode);

    // Attach the future to the watcher
    result_watcher.setFuture(future_result);
}

/*
    Slot executed automatically when background organization finishes.
*/
void MainWindow::on_organization_finished()
{
    /*
        Retrieve result from the completed background task.

        Safe because QFutureWatcher ensures synchronization.
    */
    organize_status result = result_watcher.result();

    // Handle result using enum-based switch (very clean design)
    switch (result)
    {
    case organize_status::success:
    {
        /*
                Success case:
                - Inform user
                - Offer to open the organized folder
            */

        QString q_root_path = ui->path_field->text();

        QMessageBox msgBox(this);
        msgBox.setWindowTitle("Operation Successful");
        msgBox.setText("Files are organized successfully!");

        ui->progress_bar->setVisible(false);

        msgBox.setIcon(QMessageBox::Information);

        // Update status field
        ui->result_field->setText("Files are organized successfully");

        // Add custom "Open Folder" button
        QPushButton *openButton =
            msgBox.addButton("Open Folder", QMessageBox::ActionRole);

        // Default OK button
        msgBox.addButton(QMessageBox::Ok);

        msgBox.exec();

        // If user chooses to open the folder
        if (msgBox.clickedButton() == openButton)
        {
            QDesktopServices::openUrl(
                QUrl::fromLocalFile(q_root_path)
                );
        }
        break;
    }

    case organize_status::path_not_found:
    {
        QMessageBox::information(
            this,
            "Invalid Path",
            "Given path is not valid."
            );
        ui->result_field->setText("Error, Invalid path.");
        break;
    }

    case organize_status::not_a_directory:
    {
        QMessageBox::information(
            this,
            "Error",
            "The selected path is not a directory."
            );
        ui->result_field->setText("Error, Not a Direcotry.");
        break;
    }

    case organize_status::permission_denied:
    {
        QMessageBox::information(
            this,
            "Error",
            "Permission Denied! Try running as Administrator."
            );
        ui->result_field->setText("Error! Permission Denied.");
        break;
    }

    case organize_status::directory_creation_failed:
    {
        QMessageBox::information(
            this,
            "Error",
            "Unable to create sub-directories."
            );
        ui->result_field->setText("Error! Directory creation failed.");
        break;
    }

    case organize_status::atomic_transfer_failed:
    {
        /*
                Atomic rename failed (usually cross-device issue).

                Ask user if fallback (copy + delete) is acceptable.
            */
        ui->result_field->setText("Error! File transfer failed.");

        QMessageBox::StandardButton reply =
            QMessageBox::question(
                this,
                "Cross-device move detected",
                "Some files are on a different device or disk separation.\n\n"
                "Unable to transfer files.\n"
                "Do you want to use Copy + Delete instead?",
                QMessageBox::Yes | QMessageBox::No
                );

        if (reply == QMessageBox::Yes)
        {
            QString q_root_path = ui->path_field->text();
            std::string root_path = q_root_path.toStdString();

            if (root_path.empty())
            {
                return; // Safety check
            }

            // Re-run organizer in fallback mode
            QFuture<organize_status> future_result =
                QtConcurrent::run(
                    organize_directory,
                    root_path,
                    transfer_mode::fallback_transfer_mode
                    );

            result_watcher.setFuture(future_result);
            return;
        }
        else
        {
            QMessageBox::information(
                this,
                "Cancelled",
                "Operation cancelled."
                );
            ui->result_field->setText("Error! Unable to transfer files.");
        }
        break;
    }

    case organize_status::fallback_transfer_failed:
    {
        QMessageBox::information(
            this,
            "Error",
            "Unable to perform operation."
            );
        ui->result_field->setText("Error! Unable to perform operation.");
        break;
    }

    case organize_status::unknown_error:
    {
        QMessageBox::information(
            this,
            "Error",
            "An unknown error occurred."
            );
        ui->result_field->setText("Unknown error occured!");
        break;
    }
    }

    /*
        Re-enable organize button after operation completes
        (success or failure).
    */
    ui->organize_button->setEnabled(true);

    ui->progress_bar->setVisible(false);

}

/*
    Applies a Qt stylesheet to the entire application.
*/
void MainWindow::apply_theme(QString themePath)
{
    // Open stylesheet file from Qt resources
    QFile file(themePath);

    if (file.open(QFile::ReadOnly | QFile::Text))
    {
        QTextStream ts(&file);
        QString styleSheet = ts.readAll();

        // Apply stylesheet globally
        qApp->setStyleSheet(styleSheet);

        file.close();
    }
}

/*
    Triggered when user selects Dark Theme from menu.
*/
void MainWindow::on_action_dark_theme_triggered()
{
    QString theme_path = ":/styles/dark.qss";
    apply_theme(theme_path);
}

/*
    Triggered when user selects Light Theme from menu.
*/
void MainWindow::on_action_light_theme_triggered()
{
    QString theme_path = ":/styles/light.qss";
    apply_theme(theme_path);
}

/*
    Triggered when Browse button is clicked.

    Opens directory chooser dialog and fills path field.
*/
void MainWindow::on_browse_button_clicked()
{
    QString directory_path =
        QFileDialog::getExistingDirectory(
            this,
            tr("Open Directory"),
            "/home",
            QFileDialog::ShowDirsOnly |
                QFileDialog::DontResolveSymlinks
            );

    if (!directory_path.isEmpty())
    {
        ui->path_field->setText(directory_path);
    }
}
/*
    Triggered when Clear button is clicked.

    Clears text of path field.
*/
void MainWindow::on_clear_button_clicked()
{
    ui->path_field->clear();
}

/*
    Triggered when user selects About from menu.
*/
void MainWindow::on_action_about_triggered()
{
    // Construct the message with HTML formatting for a professional look
    QString aboutText =
        "<h1>File Organizer</h1>"
        "<p>v1.0.0</p>"
        "<p>This application was built to automate directory management with safety and speed.</p>"
        "<p><b>Custom Logo:</b> Designed in Paint 3D (3D-to-2D conversion).</p>"
        "<hr>"
        "<p>Created with C++23 and Qt 6.</p>";

    QMessageBox::about(this, "About File Organizer", aboutText);
}

