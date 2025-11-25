-- mytam--1.0.sql

-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION mytam" to load this file. \quit

-- This is your existing function
CREATE FUNCTION mytam_handler(internal)
RETURNS table_am_handler
AS 'MODULE_PA THNAME'
LANGUAGE C;

-- This is your existing access method
CREATE ACCESS METHOD mytam TYPE TABLE HANDLER mytam_handler;

-- --- ADD THIS NEW FUNCTION REGISTRATION ---
CREATE FUNCTION mytam_create_table(name text)
RETURNS integer
AS 'MODULE_PATHNAME'
LANGUAGE C STRICT;
-- --- END OF ADDED LINES ---