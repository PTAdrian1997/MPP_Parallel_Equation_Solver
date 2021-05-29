import agents.QueenAgent.QueenMessageT
import akka.actor.typed.ActorRef

package object agents {

  val LoggerName: String = "QueenLogger"

  type Nogoods = Array[Nogood]
  val EmptyNogoods: Nogoods = Array.empty[Nogood]

  type LocalView = Map[Int, Int]
  type LocalViews = Array[LocalView]

  type Index = Int
  type Indexes = Vector[Index]
  val EmptyIndexes: Indexes = Vector.empty[Index]

  type ColumnValueType = Int
  type ColumnDomain = Array[Int]

  type YellowBook = Map[Int, ActorRef[QueenMessageT]]

}