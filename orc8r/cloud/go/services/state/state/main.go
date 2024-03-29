/*
 * Copyright (c) Facebook, Inc. and its affiliates.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree.
 */

package main

import (
	"time"

	"magma/orc8r/cloud/go/blobstore"
	"magma/orc8r/cloud/go/orc8r"
	"magma/orc8r/cloud/go/service"
	"magma/orc8r/cloud/go/services/state"
	"magma/orc8r/cloud/go/services/state/metrics"
	"magma/orc8r/cloud/go/services/state/servicers"
	"magma/orc8r/cloud/go/sqorc"
	"magma/orc8r/cloud/go/storage"
	"magma/orc8r/lib/go/protos"

	"github.com/golang/glog"
)

// how often to report gateway status
const gatewayStatusReportInterval = time.Second * 60

func main() {
	srv, err := service.NewOrchestratorService(orc8r.ModuleName, state.ServiceName)
	if err != nil {
		glog.Fatalf("Error creating state service %v", err)
	}
	db, err := sqorc.Open(storage.SQLDriver, storage.DatabaseSource)
	if err != nil {
		glog.Fatalf("Failed to connect to database: %v", err)
	}
	store := blobstore.NewEntStorage(state.DBTableName, db, sqorc.GetSqlBuilder())
	err = store.InitializeFactory()
	if err != nil {
		glog.Fatalf("Error initializing state database: %v", err)
	}

	server, err := servicers.NewStateServicer(store)
	if err != nil {
		glog.Fatalf("Error creating state server: %v", err)
	}
	protos.RegisterStateServiceServer(srv.GrpcServer, server)

	// Periodically go through all existing gateways and log metrics
	go metrics.PeriodicallyReportGatewayStatus(gatewayStatusReportInterval)

	err = srv.Run()
	if err != nil {
		glog.Fatalf("Error running service: %v", err)
	}
}
