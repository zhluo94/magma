/*
 Copyright (c) Facebook, Inc. and its affiliates.
 All rights reserved.

 This source code is licensed under the BSD-style license found in the
 LICENSE file in the root directory of this source tree.
*/

package sqorc

import (
	"database/sql"
	"fmt"
	"testing"

	"magma/orc8r/lib/go/definitions"

	"github.com/stretchr/testify/require"
)

const (
	// hostEnvVar is the name of the environment variable holding the DB
	// server's hostname.
	hostEnvVar = "TEST_DATABASE_HOST"
	// postgresPortEnvVar is the name of the environment variable holding the
	// Postgres container's port number.
	postgresPortEnvVar = "TEST_DATABASE_PORT_POSTGRES"
	// mariaPortEnvVar is the name of the environment variable holding the
	// Maria container's port number.
	mariaPortEnvVar = "TEST_DATABASE_PORT_MARIA"

	// postgresTestHost is the hostname of the postgres_test container
	postgresTestHost = "postgres_test"
	// mariaTestHost is the hostname of the maria_test container
	mariaTestHost = "maria_test"

	// postgresDefaultPort is the default port exposed by the postgres container.
	postgresDefaultPort = "5432"
	// mariaDefaultPort is the default port exposed by the maria container.
	mariaDefaultPort = "3306"
)

// OpenCleanForTest is the same as OpenForTest, except it also drops then
// creates the underlying DB name before returning.
func OpenCleanForTest(t *testing.T, dbName, dbDriver string) *sql.DB {
	masterDB := OpenForTest(t, "", dbDriver)
	_, err := masterDB.Exec(fmt.Sprintf("DROP DATABASE IF EXISTS %s", dbName))
	if err != nil {
		t.Fatalf("Failed to drop test DB: %s", err)
	}
	_, err = masterDB.Exec(fmt.Sprintf("CREATE DATABASE %s", dbName))
	if err != nil {
		t.Fatalf("Failed to create test DB: %s", err)
	}
	require.NoError(t, masterDB.Close())

	db := OpenForTest(t, dbName, dbDriver)
	return db
}

// OpenForTest returns a new connection to a shared test DB.
// Does not guarantee the existence of the underlying DB name.
// The shared DB is part of the shared testing infrastructure, so care must
// be taken to avoid racing on the same DB name across testing code.
// Supported DB drivers include:
//	- postgres
//	- mysql
// Environment variables:
//	- SQL_DRIVER overrides the Go SQL driver
//	- TEST_DATABASE_HOST overrides the DB connection host
//	- TEST_DATABASE_PORT_POSTGRES overrides the port connected to for postgres driver
//	- TEST_DATABASE_PORT_MARIA overrides the port connected to for maria driver
func OpenForTest(t *testing.T, dbName, dbDriver string) *sql.DB {
	driver := definitions.GetEnvWithDefault("SQL_DRIVER", dbDriver)

	host := getHost(t, driver)
	port := getPort(t, driver)
	source := fmt.Sprintf("host=%s port=%s user=magma_test password=magma_test sslmode=disable", host, port)
	if dbName != "" {
		source = fmt.Sprintf("%s dbname=%s", source, dbName)
	}

	db, err := Open(driver, source)
	if err != nil {
		t.Fatalf("Could not initialize %s DB connection: %v", driver, err)
	}
	return db
}

func getHost(t *testing.T, dbDriver string) string {
	var host string

	switch dbDriver {
	case PostgresDriver:
		host = definitions.GetEnvWithDefault(hostEnvVar, postgresTestHost)
	case MariaDriver:
		host = definitions.GetEnvWithDefault(hostEnvVar, mariaTestHost)
	default:
		t.Fatalf("Unrecognized DB driver: %s", dbDriver)
	}

	return host
}

func getPort(t *testing.T, dbDriver string) string {
	var port string

	switch dbDriver {
	case PostgresDriver:
		port = definitions.GetEnvWithDefault(postgresPortEnvVar, postgresDefaultPort)
	case MariaDriver:
		port = definitions.GetEnvWithDefault(mariaPortEnvVar, mariaDefaultPort)
	default:
		t.Fatalf("Unrecognized DB driver: %s", dbDriver)
	}

	return port
}
