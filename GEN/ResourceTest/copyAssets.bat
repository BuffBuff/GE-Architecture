@ECHO OFF
SETLOCAL

IF NOT DEFINED HAV_CONV (
	ECHO Environment variable HAV_CONV not set to the converter to use.
	EXIT /b 1
)

SET CONV=%HAV_CONV%
SET SRC_DIR=..\..\Assets
SET DEST_DIR=assets

MKDIR "%DEST_DIR%\models"
MKDIR "%DEST_DIR%\textures"

ECHO(Converting models.
CALL "%CONV%" "%SRC_DIR%\Model\Barrel1.tx" "%DEST_DIR%"
ECHO(

DEL "%DEST_DIR%\Resources.xml"

ECHO(Copying models.
MOVE /Y "%SRC_DIR%\Model\*.btx" "%DEST_DIR%\models"
ECHO(

ECHO(Copying textures.
COPY /Y %SRC_DIR%\Texture\* %DEST_DIR%\textures
