/*
 * Copyright (C) 2007 Andrea Zagli <azagli@inwind.it>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <stdlib.h>
#include <string.h>

#include <libgda/libgda.h>
#include <libxml/xpath.h>

#ifdef HAVE_CONFIG_H
	#include <config.h>
#endif

#include "rptreport.h"
#include "rptcommon.h"
#include "rptobject.h"
#include "rptobjecttext.h"

typedef struct
{
	RptSize *size;
} Page;

typedef struct
{
	gdouble height;
	GList *objects;
} Body;

enum
{
	PROP_0
};

static void rpt_report_class_init (RptReportClass *klass);
static void rpt_report_init (RptReport *rpt_report);

static void rpt_report_set_property (GObject *object,
                                     guint property_id,
                                     const GValue *value,
                                     GParamSpec *pspec);
static void rpt_report_get_property (GObject *object,
                                     guint property_id,
                                     GValue *value,
                                     GParamSpec *pspec);

static void rpt_report_xml_parse_body (RptReport *rpt_report, xmlNode *xnode);
static RptObject *rpt_report_get_object_from_name (GList *list, const gchar *name);

static xmlNode *rpt_report_rptprint_new_page (RptReport *rptreport,
                                              xmlNode *xroot);
static void rpt_report_rptprint_body (RptReport *rptreport,
                                      xmlNode *xpage,
                                      gdouble *cur_y);
static void rpt_report_rptprint_parse_source (RptReport *rptreport,
                                              RptObject *rptobj,
                                              xmlNode *xnode);


#define RPT_REPORT_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), TYPE_RPT_REPORT, RptReportPrivate))

typedef struct _RptReportPrivate RptReportPrivate;
struct _RptReportPrivate
	{
		GdaClient *gda_client;
		GdaConnection *gda_conn;
		GdaDataModel *gda_datamodel;

		Page *page;
		Body *body;
	};

GType
rpt_report_get_type (void)
{
	static GType rpt_report_type = 0;

	if (!rpt_report_type)
		{
			static const GTypeInfo rpt_report_info =
			{
				sizeof (RptReportClass),
				NULL,	/* base_init */
				NULL,	/* base_finalize */
				(GClassInitFunc) rpt_report_class_init,
				NULL,	/* class_finalize */
				NULL,	/* class_data */
				sizeof (RptReport),
				0,	/* n_preallocs */
				(GInstanceInitFunc) rpt_report_init,
				NULL
			};

			rpt_report_type = g_type_register_static (G_TYPE_OBJECT, "RptReport",
			                                          &rpt_report_info, 0);
		}

	return rpt_report_type;
}

static void
rpt_report_class_init (RptReportClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (object_class, sizeof (RptReportPrivate));

	object_class->set_property = rpt_report_set_property;
	object_class->get_property = rpt_report_get_property;
}

static void
rpt_report_init (RptReport *rpt_report)
{
	RptReportPrivate *priv = RPT_REPORT_GET_PRIVATE (rpt_report);

	priv->gda_client = NULL;
	priv->gda_conn = NULL;
	priv->gda_datamodel = NULL;

	priv->page = (Page *)g_malloc0 (sizeof (Page));
	priv->page->size = (RptSize *)g_malloc0 (sizeof (RptSize));
	priv->page->size->width = 0.0;
	priv->page->size->height = 0.0;

	priv->body = (Body *)g_malloc0 (sizeof (Body));
	priv->body->height = 0.0;
}

/**
 * rpt_report_new_from_xml:
 * @xdoc: an #xmlDoc.
 *
 * Returns: the newly created #RptReport object.
 */
RptReport
*rpt_report_new_from_xml (xmlDoc *xdoc)
{
	RptReport *rpt_report = NULL;

	xmlNode *cur = xmlDocGetRootElement (xdoc);
	if (cur != NULL)
		{
			if (strcmp (cur->name, "reptool") == 0)
				{
					xmlXPathContextPtr xpcontext;
					xmlXPathObjectPtr xpresult;
					xmlNodeSetPtr xnodeset;
					RptReportPrivate *priv;

					rpt_report = RPT_REPORT (g_object_new (rpt_report_get_type (), NULL));

					priv = RPT_REPORT_GET_PRIVATE (rpt_report);
					xpcontext = xmlXPathNewContext (xdoc);

					/* search for node "page" */
					xpcontext->node = cur;
					xpresult = xmlXPathEvalExpression ((const xmlChar *)"child::page", xpcontext);
					if (!xmlXPathNodeSetIsEmpty (xpresult->nodesetval))
						{
							xnodeset = xpresult->nodesetval;
							if (xnodeset->nodeNr == 1)
								{
									gchar *prop;
									prop = xmlGetProp (xnodeset->nodeTab[0], (const xmlChar *)"width");
									if (prop != NULL)
										{
											priv->page->size->width = strtod (prop, NULL);
										}
									prop = xmlGetProp (xnodeset->nodeTab[0], (const xmlChar *)"height");
									if (prop != NULL)
										{
											priv->page->size->height = strtod (prop, NULL);
										}
								}
							else
								{
									/* TO DO */
									/* return */
								}
						}
					else
						{
							/* TO DO */
							/* return */
						}

					/* search for node "database" */
					xpcontext->node = cur;
					xpresult = xmlXPathEvalExpression ((const xmlChar *)"child::database", xpcontext);
					if (!xmlXPathNodeSetIsEmpty (xpresult->nodesetval))
						{
							xnodeset = xpresult->nodesetval;
							if (xnodeset->nodeNr == 1)
								{
									gchar *provider;
									gchar *connection_string;
									gchar *sql;
								
									xmlNode *cur_database = xnodeset->nodeTab[0]->children;
									while (cur_database != NULL)
										{
											if (strcmp (cur_database->name, "provider") == 0)
												{
													provider = g_strstrip ((gchar *)xmlNodeGetContent (cur_database));
												}
											else if (strcmp (cur_database->name, "connection_string") == 0)
												{
													connection_string = g_strstrip ((gchar *)xmlNodeGetContent (cur_database));
												}
											else if (strcmp (cur_database->name, "sql") == 0)
												{
													sql = g_strstrip ((gchar *)xmlNodeGetContent (cur_database));
												}
											
											cur_database = cur_database->next;
										}

									if (strcmp (provider, "") == 0 ||
										strcmp (connection_string, "") == 0 ||
										strcmp (sql, "") == 0)
										{
											/* TO DO */
										}
									else
										{
											/* database connection */
											gda_init (PACKAGE_NAME, PACKAGE_VERSION, 0, NULL);
											priv->gda_client = gda_client_new ();
											priv->gda_conn = gda_client_open_connection_from_string (priv->gda_client,
																									 provider,
																									 connection_string,
																									 0);
											if (priv->gda_conn == NULL)
												{
													/* TO DO */
													g_warning ("Unable to establish the connection.");
												}
											else
												{
													GdaCommand *command = gda_command_new (sql, GDA_COMMAND_TYPE_SQL, GDA_COMMAND_OPTION_STOP_ON_ERRORS);

													priv->gda_datamodel = gda_connection_execute_single_command (priv->gda_conn, command, NULL);

													gda_command_free (command);
												}
										}
								}
						}

					/* search for node "report" */
					xpcontext->node = cur;
					xpresult = xmlXPathEvalExpression ((const xmlChar *)"child::report", xpcontext);
					if (!xmlXPathNodeSetIsEmpty (xpresult->nodesetval))
						{
							xnodeset = xpresult->nodesetval;
							if (xnodeset->nodeNr == 1)
								{
									/* search for node "body" */
									xpcontext->node = xnodeset->nodeTab[0];
									xpresult = xmlXPathEvalExpression ((const xmlChar *)"child::body", xpcontext);
									if (!xmlXPathNodeSetIsEmpty (xpresult->nodesetval))
										{
											xnodeset = xpresult->nodesetval;
											if (xnodeset->nodeNr == 1)
												{
													rpt_report_xml_parse_body (rpt_report, xnodeset->nodeTab[0]);
												}
										}
								}
							else
								{
									/* TO DO */
									/* return */
								}
						}
					else
						{
							/* TO DO */
							/* return */
						}
				}
			else
				{
					/* TO DO */
				}
		}

	return rpt_report;
}

/**
 * rpt_report_new_from_file:
 * @filename: the path of the xml file to load.
 *
 * Returns: the newly created #RptReport object.
 */
RptReport
*rpt_report_new_from_file (const gchar *filename)
{
	RptReport *rpt_report = NULL;

	xmlDoc *xdoc = xmlParseFile (filename);
	if (xdoc != NULL)
		{
			rpt_report = rpt_report_new_from_xml (xdoc);
		}

	return rpt_report;
}

/**
 * rpt_report_get_xml:
 * @rptreport: an #RptReport object.
 *
 */
xmlDoc
*rpt_report_get_xml (RptReport *rptreport)
{
	xmlDoc *xdoc;

	RptReportPrivate *priv = RPT_REPORT_GET_PRIVATE (rptreport);

	/* TO DO */

	return xdoc;
}

/**
 * rpt_report_get_xml_rptprint:
 * @rptreport: an #RptReport object.
 *
 */
xmlDoc
*rpt_report_get_xml_rptprint (RptReport *rptreport)
{
	xmlDoc *xdoc;
	xmlNode *xroot;
	xmlNode *xpage;
	gint pages;
	gdouble cur_y = 0.0;

	RptReportPrivate *priv = RPT_REPORT_GET_PRIVATE (rptreport);

	xdoc = xmlNewDoc ("1.0");
	xroot = xmlNewNode (NULL, "reptool_report");
	xmlDocSetRootElement (xdoc, xroot);

	if (priv->gda_datamodel != NULL)
		{
			gint row;
			gint rows;

			rows = gda_data_model_get_n_rows (priv->gda_datamodel);

			for (row = 0; row < rows; row++)
				{
					if (row == 0 || cur_y > priv->page->size->height)
						{
							cur_y = 0.0;
							pages++;
							xpage = rpt_report_rptprint_new_page (rptreport, xroot);
						}

					rpt_report_rptprint_body (rptreport, xpage, &cur_y);
				}
		}
	else
		{
			pages++;
			xpage = rpt_report_rptprint_new_page (rptreport, xroot);
			rpt_report_rptprint_body (rptreport, xpage, &cur_y);
		}

	return xdoc;
}

static void
rpt_report_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	RptReport *rpt_report = RPT_REPORT (object);

	RptReportPrivate *priv = RPT_REPORT_GET_PRIVATE (rpt_report);

	switch (property_id)
		{
			default:
				G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
				break;
		}
}

static void
rpt_report_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	RptReport *rpt_report = RPT_REPORT (object);

	RptReportPrivate *priv = RPT_REPORT_GET_PRIVATE (rpt_report);

	switch (property_id)
		{
			default:
				G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
				break;
		}
}

static void
rpt_report_xml_parse_body (RptReport *rpt_report, xmlNode *xnode)
{
	RptObject *rptobj;
	gchar *prop;
	gchar *objname;
	xmlNode *cur;

	RptReportPrivate *priv = RPT_REPORT_GET_PRIVATE (rpt_report);

	prop = (gchar *)xmlGetProp (xnode, "height");
	if (prop != NULL)
		{
			priv->body->height = strtod (g_strstrip (g_strdup (prop)), NULL);
		}

	cur = xnode->children;
	while (cur != NULL)
		{
			if (strcmp (cur->name, "text") == 0)
				{
					rptobj = rpt_obj_text_new_from_xml (cur);
					if (rptobj != NULL)
						{
							g_object_get (rptobj, "name", &objname, NULL);

							if (rpt_report_get_object_from_name (priv->body->objects, objname) == NULL)
								{
									priv->body->objects = g_list_append (priv->body->objects, rptobj);
								}
							else
								{
									/* TO DO */
									g_warning ("An object with name \"%s\" already exists.", objname);
								}
						}
				}
			else if (strcmp (cur->name, "line") == 0)
				{
				}
			else if (strcmp (cur->name, "rect") == 0)
				{
				}
			else if (strcmp (cur->name, "image") == 0)
				{
				}

			cur = cur->next;
		}
}

static RptObject
*rpt_report_get_object_from_name (GList *list, const gchar *name)
{
	gchar *objname;
	RptObject *obj = NULL;

	list = g_list_first (list);
	while (list != NULL)
		{
			g_object_get ((RptObject *)list->data, "name", &objname, NULL);
			if (strcmp (name, objname) == 0)
				{
					obj = (RptObject *)list->data;
					break;
				}
		
			list = g_list_next (list);
		}

	return obj;
}

static xmlNode
*rpt_report_rptprint_new_page (RptReport *rptreport, xmlNode *xroot)
{
	xmlNode *xnode;

	RptReportPrivate *priv = RPT_REPORT_GET_PRIVATE (rptreport);

	xnode = xmlNewNode (NULL, "page");
	xmlSetProp (xnode, "width", g_strdup_printf ("%f", priv->page->size->width));
	xmlSetProp (xnode, "height", g_strdup_printf ("%f", priv->page->size->height));
	xmlAddChild (xroot, xnode);

	return xnode;
}

static void
rpt_report_rptprint_body (RptReport *rptreport, xmlNode *xpage, gdouble *cur_y)
{
	GList *objects;
	xmlAttrPtr attr;
	xmlNode *xnode;
	gchar *prop;

	RptObject *rptobj;

	RptReportPrivate *priv = RPT_REPORT_GET_PRIVATE (rptreport);

	objects = g_list_first (priv->body->objects);
	while (objects != NULL)
		{
			xnode = xmlNewNode (NULL, "node");

			rptobj = (RptObject *)objects->data;
			rpt_object_get_xml (rptobj, xnode);
			attr = xmlHasProp (xnode, "name");
			if (attr != NULL)
				{
					xmlRemoveProp (attr);
				}

			prop = (gchar *)xmlGetProp (xnode, "y");
			if (prop == NULL)
				{
					prop = g_strdup ("0.0");
				}
			xmlSetProp (xnode, "y", g_strdup_printf ("%f", strtod (prop, NULL) + *cur_y));

			if (IS_RPT_OBJ_TEXT (rptobj))
				{
					rpt_report_rptprint_parse_source (rptreport, rptobj, xnode);
				}

			xmlAddChild (xpage, xnode);

			objects = g_list_next (objects);
		}
	*cur_y += priv->body->height;
}

static void
rpt_report_rptprint_parse_source (RptReport *rptreport, RptObject *rptobj, xmlNode *xnode)
{
	/* TO DO */
	gchar *source;

	g_object_get (G_OBJECT (rptobj), "source", &source, NULL);

	xmlNodeSetContent (xnode, source);
}