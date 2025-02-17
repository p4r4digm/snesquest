#include "DB.h"

#include "libutils/String.h"
#include "libutils/CheckedMemory.h"
#include "libutils/BitBuffer.h"

#include "sqlite/sqlite3.h"

int dbConnect(DBBase *self, const char *filename, boolean create) {
   int result = 0;
   int flags = SQLITE_OPEN_READWRITE;
   if (create) {
      flags |= SQLITE_OPEN_CREATE;
   }

   if (!self->err) {
      self->err = stringCreate("");
   }
   else {
      stringSet(self->err, "");
   }

   if (!self || self->open) {
      if (stringEqualRaw(self->dbPath, filename)) {
         //attempting to reconnect to existing db, no-op
         return DB_SUCCESS;
      }
      else {
         //already open bro
         stringSet(self->err, "Database already open.");
         return DB_FAILURE;
      }
   }

   result = sqlite3_open_v2(filename, &self->conn, flags, NULL);
   if (result != SQLITE_OK) {
      stringSet(self->err, sqlite3_errmsg(self->conn));
      sqlite3_close(self->conn);
      self->conn = NULL;
      return DB_FAILURE;
   }

   if (!self->dbPath) {
      self->dbPath = stringCreate(filename);
   }
   else {
      stringSet(self->dbPath, filename);
   }

   self->open = true;
   return DB_SUCCESS;
}
boolean _dbIsConnected(DBBase *self) {
   return self->open;
}

int dbDisconnect(DBBase *self) {
   if (!self || !self->open) {
      stringSet(self->err, "No db connection.");
      return DB_FAILURE;
   }

   sqlite3_close(self->conn);

   self->open = false;
   self->conn = NULL;
   return DB_SUCCESS;
}
void dbDestroy(DBBase *self) {
   if (self) {
      if (self->open) {
         dbDisconnect(self);
      }
      stringDestroy(self->dbPath);
      stringDestroy(self->err);
   }

}
const char *_dbGetError(DBBase *self) {
   if (!stringLen(self->err)) {
      return NULL;
   }

   return c_str(self->err);
}

void _dbClearError(DBBase *self) {
   stringClear(self->err);
}

int _dbBeginTransaction(DBBase *self) {
   return dbExecute(self, "BEGIN TRANSACTION;");
}
int _dbRollbackTransaction(DBBase *self) {
   return dbExecute(self, "ROLLBACK TRANSACTION;");
}
int _dbCommitTransaction(DBBase *self) {
   return dbExecute(self, "COMMIT TRANSACTION;");
}

int _dbExecute(DBBase *self, const char *cmd) {
   int result = 0;
   char *err = NULL;

   if (!self->open) {
      stringSet(self->err, "Not DB connection.");
      return DB_FAILURE;
   }

   result = sqlite3_exec(self->conn, cmd, NULL, NULL, &err);

   if (result != SQLITE_OK) {
      stringSet(self->err, err);
      free(err);
      return DB_FAILURE;
   }

   return DB_SUCCESS;
}
int dbPrepareStatement(DBBase *self, sqlite3_stmt **out, const char *stmt) {
   if (!self || !self->open) {
      stringSet(self->err, "No db connection.");
      return DB_FAILURE;
   }

   if (!*out) {
      sqlite3_stmt *sqlstmt = NULL;
      int result = sqlite3_prepare_v2(self->conn, stmt, strlen(stmt) + 1, &sqlstmt, NULL);
      if (result != SQLITE_OK) {
         stringSet(self->err, sqlite3_errmsg(self->conn));
         return DB_FAILURE;
      }
      *out = sqlstmt;
   }

   sqlite3_reset(*out);
   return DB_SUCCESS;
}
