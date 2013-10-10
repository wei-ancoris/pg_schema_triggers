-- Create an event trigger for the column_alter event.
CREATE EXTENSION pg_schema_triggers;
CREATE OR REPLACE FUNCTION public.on_column_alter()
 RETURNS event_trigger
 LANGUAGE plpgsql
 AS $$
	DECLARE
		event_info SCHEMA_TRIGGERS.COLUMN_ALTER_EVENTINFO;
	BEGIN
		event_info := schema_triggers.get_column_alter_eventinfo();
		RAISE NOTICE 'on_column_alter(%, %)', event_info.relation, event_info.attnum;
		RAISE NOTICE '  old.attname=''%'', old.atttypid=%, old.attnotnull=''%''',
	 		(event_info.old).attname, (event_info.old).atttypid, (event_info.old).attnotnull;
		RAISE NOTICE '  new.attname=''%'', new.atttypid=%, new.attnotnull=''%''',
	 		(event_info.new).attname, (event_info.new).atttypid, (event_info.new).attnotnull;
	END;
 $$;
CREATE EVENT TRIGGER colalter ON column_alter
	EXECUTE PROCEDURE on_column_alter();

-- Create some tables, and then alter their columns.
CREATE TABLE foobar();
ALTER TABLE foobar ADD COLUMN x TEXT NOT NULL;
ALTER TABLE foobar RENAME COLUMN x TO xxx;
ALTER TABLE foobar ALTER COLUMN xxx DROP NOT NULL;
ALTER TABLE foobar ALTER COLUMN xxx TYPE BOOLEAN USING (FALSE);
DROP TABLE foobar;
CREATE TABLE baz(a INTEGER PRIMARY KEY, b TEXT, c BOOLEAN);
ALTER TABLE baz DROP COLUMN c;
ALTER TABLE baz ALTER COLUMN b SET NOT NULL;
ALTER TABLE baz RENAME COLUMN a TO aaa;
DROP TABLE baz;
CREATE TABLE xyzzy(abc INTEGER, def INTEGER, ghi INTEGER);
ALTER TABLE xyzzy
	ALTER COLUMN abc TYPE TEXT,
	ADD COLUMN jkl BOOLEAN NOT NULL,
	ALTER COLUMN ghi SET NOT NULL;
DROP TABLE xyzzy;

-- Clean up the event trigger.
DROP EVENT TRIGGER colalter;
DROP FUNCTION on_column_alter();
DROP EXTENSION pg_schema_triggers;
