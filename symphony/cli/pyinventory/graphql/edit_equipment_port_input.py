#!/usr/bin/env python3
# @generated AUTOGENERATED file. Do not Change!

from dataclasses import dataclass
from datetime import datetime
from functools import partial
from gql.gql.datetime_utils import DATETIME_FIELD
from numbers import Number
from typing import Any, Callable, List, Mapping, Optional

from dataclasses_json import DataClassJsonMixin

from .link_side_input import LinkSide
from .property_input import PropertyInput
@dataclass
class EditEquipmentPortInput(DataClassJsonMixin):
    side: LinkSide
    properties: List[PropertyInput]
