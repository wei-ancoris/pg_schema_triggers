/*
 * Event functions that are called from pg_schema_triggers.c when various
 * interesting database events happen.
 *
 * These functions will (usually) set up an EventContext struct and then
 * invoke FireEventTriggers().
 *
 * pg_schema_triggers/events.c
 */


#include "postgres.h"
#include "fmgr.h"
#include "funcapi.h"
#include "access/hash.h"
#include "access/htup_details.h"
#include "access/xact.h"
#include "catalog/objectaccess.h"
#include "catalog/objectaddress.h"
#include "catalog/pg_class.h"
#include "catalog/pg_type.h"
#include "parser/parse_func.h"
#include "storage/itemptr.h"
#include "tcop/utility.h"
#include "utils/builtins.h"
#include "utils/lsyscache.h"
#include "utils/rel.h"
#include "utils/tqual.h"


#include "catalog_funcs.h"
#include "events.h"
#include "trigger_funcs.h"


/*** Event:  "listen" ***/


void
listen_event(const char *condition_name)
{
	FireEventTriggers("listen", condition_name, NULL);
}


/*** Event:  "relation.create" ***/


typedef struct RelationCreate_EventInfo {
	char *eventname;
	Oid relnamespace;
	Oid relation;
	char relkind;
} RelationCreate_EventInfo;


void
relation_create_event(ObjectAddress *rel)
{
	RelationCreate_EventInfo info;
	char *nspname;
	char *relname;
	char *tag;

	Assert(rel->classId == RelationRelationId);

	/* Bump the command counter so we can see the newly-created relation. */
	CommandCounterIncrement();

	/* Set up the event info. */
	info.eventname = "relation.create";
	info.relation = rel->objectId;
	info.relnamespace = get_object_namespace(rel);
	info.relkind = get_rel_relkind(info.relation);

	/* Prepare the tag string. */
	nspname = get_namespace_name(info.relnamespace);
	relname = get_rel_name(rel->objectId);
	tag = quote_qualified_identifier(nspname, relname); 

	/* Fire the trigger. */
	FireEventTriggers("relation.create", tag, (EventInfo*)&info);
}


PG_FUNCTION_INFO_V1(relation_create_eventinfo);
Datum
relation_create_eventinfo(PG_FUNCTION_ARGS)
{
	RelationCreate_EventInfo *info;
	TupleDesc tupdesc;
	Datum result[3];
	bool result_isnull[3];
	HeapTuple tuple;
	
	/* Extract the information from our EventInfo struct. */
	info = (RelationCreate_EventInfo *)GetCurrentEventInfo("relation.create");
	result[0] = ObjectIdGetDatum(info->relation);
	result[1] = ObjectIdGetDatum(info->relnamespace);
	result[2] = CharGetDatum(info->relkind);
	result_isnull[0] = false;
	result_isnull[1] = false;
	result_isnull[2] = false;

	/* Build our composite result and return it. */
	if (get_call_result_type(fcinfo, NULL, &tupdesc) != TYPEFUNC_COMPOSITE)
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg("function returning record called in context "
				        "that cannot accept type record")));
	BlessTupleDesc(tupdesc);
	tuple = heap_form_tuple(tupdesc, result, result_isnull);
	PG_RETURN_DATUM(HeapTupleGetDatum(tuple));
}


/*** Event:  "relation.alter" ***/


typedef struct RelationAlter_EventInfo {
	char *eventname;
	Oid relation;
	Oid relnamespace;
	HeapTuple old;
	HeapTuple new;
} RelationAlter_EventInfo;


void
relation_alter_event(ObjectAddress *rel)
{
	RelationAlter_EventInfo info;
	char *nspname;
	char *relname;
	char *tag;

	Assert(rel->classId == RelationRelationId);

	/* Set up the event info and save the old and new pg_class rows. */
	info.eventname = "relation.alter";
	info.relation = rel->objectId;
	info.relnamespace = get_object_namespace(rel);
	info.old = pgclass_fetch_tuple(rel->objectId, SnapshotNow);
	info.new = pgclass_fetch_tuple(rel->objectId, SnapshotSelf);
	if (!HeapTupleIsValid(info.old))
		elog(ERROR, "couldn't find old pg_class row for oid=(%u)", rel->objectId);
	if (!HeapTupleIsValid(info.new))
		elog(ERROR, "couldn't find new pg_class row for oid=(%u)", rel->objectId);

	/* Prepare the tag string. */
	nspname = get_namespace_name(info.relnamespace);
	relname = get_rel_name(rel->objectId);
	tag = quote_qualified_identifier(nspname, relname); 

	/* Fire the trigger. */
	FireEventTriggers("relation.alter", tag, (EventInfo*)&info);

	/* Free the old and new tuples. */
	heap_freetuple(info.old);
	heap_freetuple(info.new);
}


PG_FUNCTION_INFO_V1(relation_alter_eventinfo);
Datum
relation_alter_eventinfo(PG_FUNCTION_ARGS)
{
	RelationAlter_EventInfo *info;
	TupleDesc tupdesc;
	Datum result[3];
	bool result_isnull[3];
	HeapTuple tuple;
	
	/* Get the tupdesc for our return type. */
	if (get_call_result_type(fcinfo, NULL, &tupdesc) != TYPEFUNC_COMPOSITE)
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg("function returning record called in context "
				        "that cannot accept type record")));
	BlessTupleDesc(tupdesc);

	/* Get our EventInfo struct. */
	info = (RelationAlter_EventInfo *)GetCurrentEventInfo("relation.alter");

	/* Form and return the tuple. */
	result[0] = ObjectIdGetDatum(info->relation);
	result[1] = HeapTupleGetDatum(info->old);
	result[2] = HeapTupleGetDatum(info->new);
	result_isnull[0] = false;
	result_isnull[1] = false;
	result_isnull[2] = false;
	tuple = heap_form_tuple(tupdesc, result, result_isnull);
	PG_RETURN_DATUM(HeapTupleGetDatum(tuple));
}
