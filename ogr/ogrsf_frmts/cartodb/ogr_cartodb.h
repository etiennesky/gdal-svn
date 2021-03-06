/******************************************************************************
 * $Id$
 *
 * Project:  CARTODB Translator
 * Purpose:  Definition of classes for OGR CartoDB driver.
 * Author:   Even Rouault, even dot rouault at mines dash paris dot org
 *
 ******************************************************************************
 * Copyright (c) 2013, Even Rouault <even dot rouault at mines-paris dot org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 ****************************************************************************/

#ifndef _OGR_CARTODB_H_INCLUDED
#define _OGR_CARTODB_H_INCLUDED

#include "ogrsf_frmts.h"
#include "cpl_http.h"

#include <vector>
#include <json.h>

json_object* OGRCARTODBGetSingleRow(json_object* poObj);
CPLString OGRCARTODBEscapeIdentifier(const char* pszStr);
CPLString OGRCARTODBEscapeLiteral(const char* pszStr);

/************************************************************************/
/*                      OGRCartoDBGeomFieldDefn                         */
/************************************************************************/

class OGRCartoDBGeomFieldDefn: public OGRGeomFieldDefn
{
    public:
        int nSRID;

        OGRCartoDBGeomFieldDefn(const char* pszName, OGRwkbGeometryType eType) :
                OGRGeomFieldDefn(pszName, eType), nSRID(0)
        {
        }
};

/************************************************************************/
/*                           OGRCARTODBLayer                            */
/************************************************************************/
class OGRCARTODBDataSource;

class OGRCARTODBLayer : public OGRLayer
{
protected:
    OGRCARTODBDataSource* poDS;

    OGRFeatureDefn      *poFeatureDefn;
    OGRSpatialReference *poSRS;
    CPLString            osBaseSQL;
    CPLString            osFIDColName;

    int                  bEOF;
    int                  nFetchedObjects;
    int                  iNextInFetchedObjects;
    int                  iNext;
    json_object         *poCachedObj;

    OGRFeature          *GetNextRawFeature();
    OGRFeature          *BuildFeature(json_object* poRowObj);

    void                 EstablishLayerDefn(const char* pszLayerName);
    OGRSpatialReference *GetSRS(const char* pszGeomCol, int *pnSRID);
    virtual CPLString    GetSRS_SQL(const char* pszGeomCol) = 0;

  public:
                         OGRCARTODBLayer(OGRCARTODBDataSource* poDS);
                        ~OGRCARTODBLayer();

    virtual void                ResetReading();
    virtual OGRFeature *        GetNextFeature();

    virtual OGRFeatureDefn *    GetLayerDefn();

    virtual int                 TestCapability( const char * );

    int                         GetFeaturesToFetch() { return atoi(CPLGetConfigOption("CARTODB_PAGE_SIZE", "500")); }
};


/************************************************************************/
/*                        OGRCARTODBTableLayer                          */
/************************************************************************/

class OGRCARTODBTableLayer : public OGRCARTODBLayer
{
    CPLString           osName;
    CPLString           osQuery;
    CPLString           osWHERE;

    int                 bInTransaction;
    CPLString           osTransactionSQL;
    long                nNextFID;

    void                BuildWhere();

    virtual CPLString    GetSRS_SQL(const char* pszGeomCol);

  public:
                         OGRCARTODBTableLayer(OGRCARTODBDataSource* poDS, const char* pszName);
                        ~OGRCARTODBTableLayer();

    virtual const char*         GetName() { return osName.c_str(); }
    virtual OGRFeatureDefn *    GetLayerDefn();

    virtual int                 GetFeatureCount( int bForce = TRUE );
    virtual OGRFeature         *GetFeature( long nFeatureId );

    virtual int                 TestCapability( const char * );

    virtual OGRErr      CreateField( OGRFieldDefn *poField,
                                     int bApproxOK = TRUE );

    virtual OGRErr      CreateFeature( OGRFeature *poFeature );
    virtual OGRErr      SetFeature( OGRFeature *poFeature );
    virtual OGRErr      DeleteFeature( long nFID );

    virtual void        SetSpatialFilter( OGRGeometry *poGeom ) { SetSpatialFilter(0, poGeom); }
    virtual void        SetSpatialFilter( int iGeomField, OGRGeometry *poGeom );
    virtual OGRErr      SetAttributeFilter( const char * );

    virtual OGRErr      GetExtent( OGREnvelope *psExtent, int bForce ) { return GetExtent(0, psExtent, bForce); }
    virtual OGRErr      GetExtent( int iGeomField, OGREnvelope *psExtent, int bForce );
    
    virtual OGRErr      StartTransaction();
    virtual OGRErr      CommitTransaction();
    virtual OGRErr      RollbackTransaction();
};

/************************************************************************/
/*                       OGRCARTODBResultLayer                          */
/************************************************************************/

class OGRCARTODBResultLayer : public OGRCARTODBLayer
{
    virtual CPLString    GetSRS_SQL(const char* pszGeomCol);

  public:
                        OGRCARTODBResultLayer( OGRCARTODBDataSource* poDS,
                                               const char * pszRawStatement );
    virtual             ~OGRCARTODBResultLayer();

    virtual OGRFeatureDefn *    GetLayerDefn();
};

/************************************************************************/
/*                           OGRCARTODBDataSource                       */
/************************************************************************/

class OGRCARTODBDataSource : public OGRDataSource
{
    char*               pszName;
    char*               pszAccount;

    OGRLayer**          papoLayers;
    int                 nLayers;

    int                 bReadWrite;

    int                 bUseHTTPS;

    CPLString           osAPIKey;

    int                 bMustCleanPersistant;

    int                 FetchSRSId( OGRSpatialReference * poSRS );

  public:
                        OGRCARTODBDataSource();
                        ~OGRCARTODBDataSource();

    int                 Open( const char * pszFilename,
                              int bUpdate );

    virtual const char* GetName() { return pszName; }

    virtual int         GetLayerCount() { return nLayers; }
    virtual OGRLayer*   GetLayer( int );
    virtual OGRLayer    *GetLayerByName(const char *);

    virtual int         TestCapability( const char * );

    virtual OGRLayer   *ICreateLayer( const char *pszName,
                                     OGRSpatialReference *poSpatialRef = NULL,
                                     OGRwkbGeometryType eGType = wkbUnknown,
                                     char ** papszOptions = NULL );
    virtual OGRErr      DeleteLayer(int);

    virtual OGRLayer *  ExecuteSQL( const char *pszSQLCommand,
                                    OGRGeometry *poSpatialFilter,
                                    const char *pszDialect );
    virtual void        ReleaseResultSet( OGRLayer * poLayer );

    const char*                 GetAPIURL() const;
    int                         IsReadWrite() const { return bReadWrite; }
    char**                      AddHTTPOptions(char** papszOptions = NULL);
    json_object*                RunSQL(const char* pszUnescapedSQL);

    int                         IsAuthenticatedConnection() { return osAPIKey.size() != 0; }
};

/************************************************************************/
/*                           OGRCARTODBDriver                           */
/************************************************************************/

class OGRCARTODBDriver : public OGRSFDriver
{
  public:
                ~OGRCARTODBDriver();

    virtual const char*         GetName();
    virtual OGRDataSource*      Open( const char *, int );

    virtual int                 TestCapability( const char * ) { return FALSE; }
};

#endif /* ndef _OGR_CARTODB_H_INCLUDED */
