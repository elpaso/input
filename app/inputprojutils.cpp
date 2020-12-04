/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "inputprojutils.h"

#include <QDebug>
#include <QDir>
#include <QFileInfo>

#include "inpututils.h"
#include "proj.h"
#include "qgsprojutils.h"
#include "inputhelp.h"

InputProjUtils::InputProjUtils( QObject *parent )
  : QObject( parent )
{
  initCoordinateOperationHandlers();
}

void InputProjUtils::warnUser( const QString &message )
{
  if ( !mPopUpShown )
  {
    emit projError( message );
  }
}

void InputProjUtils::logUser( const QString &message, bool &variable )
{
  if ( !variable )
  {
    InputUtils::log( "InputPROJ", message );
    variable = true;
  }
}

void InputProjUtils::cleanCustomDir()
{
  QDir dir( mCurrentCustomProjDir );
  if ( !dir.isEmpty() )
  {
    qDebug() << "InputPROJ: cleaning custom proj dir " << mCurrentCustomProjDir;
    dir.removeRecursively();
  }
}

static QStringList detailsToStr( const QgsDatumTransform::TransformDetails &details )
{
  QStringList messages;
  for ( const QgsDatumTransform::GridDetails &grid : details.grids )
  {
    if ( !grid.isAvailable )
    {
      messages.append( grid.shortName );
    }
  }
  return messages;
}

void InputProjUtils::initCoordinateOperationHandlers()
{
  QgsCoordinateTransform::setCustomMissingRequiredGridHandler( [ = ]( const QgsCoordinateReferenceSystem & sourceCrs,
      const QgsCoordinateReferenceSystem & destinationCrs,
      const QgsDatumTransform::GridDetails & grid )
  {
    Q_UNUSED( destinationCrs )
    Q_UNUSED( sourceCrs )
    logUser( QStringLiteral( "missing required grid: %1" ).arg( grid.shortName ), mMissingRequiredGridReported );
    warnUser( tr( "Missing required PROJ datum shift grid: %1." ).arg( grid.shortName ) );
  } );

  QgsCoordinateTransform::setCustomMissingPreferredGridHandler( [ = ]( const QgsCoordinateReferenceSystem & sourceCrs,
      const QgsCoordinateReferenceSystem & destinationCrs,
      const QgsDatumTransform::TransformDetails & preferredOperation,
      const QgsDatumTransform::TransformDetails & availableOperation )
  {
    Q_UNUSED( destinationCrs )
    Q_UNUSED( sourceCrs )
    Q_UNUSED( availableOperation )
    logUser( QStringLiteral( "missing preffered grid: %1" ).arg( detailsToStr( preferredOperation ).join( ";" ) ), mMissingPreferredGridReported );
  } );

  QgsCoordinateTransform::setCustomCoordinateOperationCreationErrorHandler( [ = ]( const QgsCoordinateReferenceSystem & sourceCrs,
      const QgsCoordinateReferenceSystem & destinationCrs,
      const QString & error )
  {
    Q_UNUSED( destinationCrs )
    Q_UNUSED( sourceCrs )
    logUser( QStringLiteral( "coordinate operation creation error: %1" ).arg( error ), mCoordinateOperationCreationErrorReported );
    warnUser( tr( "Error creating custom PROJ operation." ) );
  } );

  QgsCoordinateTransform::setCustomMissingGridUsedByContextHandler( [ = ]( const QgsCoordinateReferenceSystem & sourceCrs,
      const QgsCoordinateReferenceSystem & destinationCrs,
      const QgsDatumTransform::TransformDetails & desired )
  {
    Q_UNUSED( destinationCrs )
    Q_UNUSED( sourceCrs )
    logUser( QStringLiteral( "custom missing grid used by context handler %1" ).arg( detailsToStr( desired ).join( ";" ) ), mMissingGridUsedByContextHandlerReported );
    warnUser( tr( "Missing required PROJ datum shift grids: %1" ).arg( detailsToStr( desired ).join( "<br>" ) ) );
  } );

  QgsCoordinateTransform::setFallbackOperationOccurredHandler( [ = ]( const QgsCoordinateReferenceSystem & sourceCrs,
      const QgsCoordinateReferenceSystem & destinationCrs,
      const QString & desired )
  {
    Q_UNUSED( destinationCrs )
    Q_UNUSED( sourceCrs )
    logUser( QStringLiteral( "fallbackOperationOccurredReported: %1" ).arg( desired ), mFallbackOperationOccurredReported );
  } );
}

static void _updateProj( const QStringList &searchPaths )
{
  char **newPaths = new char *[searchPaths.count()];
  for ( int i = 0; i < searchPaths.count(); ++i )
  {
    newPaths[i] = strdup( searchPaths.at( i ).toUtf8().constData() );
  }
  proj_context_set_search_paths( nullptr, searchPaths.count(), newPaths );
  for ( int i = 0; i < searchPaths.count(); ++i )
  {
    free( newPaths[i] );
  }
  delete [] newPaths;
}


void InputProjUtils::initProjLib( const QString &pkgPath )
{
#ifdef MOBILE_OS
#ifdef ANDROID
  // win and ios resources are already in the bundle
  InputUtils::cpDir( "assets:/qgis-data", pkgPath );
  QString prefixPath = pkgPath + "/proj";
  QString projFilePath = prefixPath + "/proj.db";
#endif

#ifdef Q_OS_IOS
  QString prefixPath = pkgPath + "/proj";
  QString projFilePath = prefixPath + "/proj.db";
#endif

#ifdef Q_OS_WIN32
  QString prefixPath = pkgPath + "\\proj";
  QString projFilePath = prefixPath + "\\proj.db";
#endif

  QFile projdb( projFilePath );
  if ( !projdb.exists() )
  {
    InputUtils::log( QStringLiteral( "PROJ6 error" ), QStringLiteral( "The Input has failed to load PROJ6 database." ) );
  }
  QStringList paths = {prefixPath};

#else
  // proj share lib is set from the proj installation on the desktop,
  // so it should work without any modifications.
  // to test check QgsProjUtils.searchPaths() in QGIS Python Console
  QStringList paths = QgsProjUtils::searchPaths();
  QString prefixPath = pkgPath + "/proj";
#endif

  mCurrentCustomProjDir = prefixPath + "_custom";
  qDebug() << "InputPROJ: Default Search Paths" << paths;
  qDebug() << "InputPROJ: Custom Search Path" << mCurrentCustomProjDir;

  cleanCustomDir();

  paths.append( mCurrentCustomProjDir );
  _updateProj( paths );
}


void InputProjUtils::modifyProjPath( const QString &projectFile )
{
  mPopUpShown = false;
  mMissingRequiredGridReported = false;
  mMissingPreferredGridReported = false;
  mCoordinateOperationCreationErrorReported = false;
  mMissingGridUsedByContextHandlerReported = false;
  mFallbackOperationOccurredReported = false;

  // DO NOT remove all files here
  // it would fail this situation
  // project A => uses grid G
  // switch to project B => do not use any grids
  // switch back to project A => QGIS from proj's context raises setCustomMissingGridUsedByContextHandler

  if ( !projectFile.isEmpty() )
  {
    QFileInfo fi( projectFile );
    QDir projDir( fi.absoluteDir().absolutePath() + "/proj" );
    if ( projDir.isReadable() && !projDir.isEmpty() )
    {
      bool success = InputUtils::cpDir( projDir.absolutePath(), mCurrentCustomProjDir );
      if ( success )
        qDebug() << "InputPROJ: updated custom proj dir with" << projDir.absolutePath();
      else
        qDebug() << "InputPROJ: failed to update custom proj dir with" << projDir.absolutePath();
    }
  }
}
